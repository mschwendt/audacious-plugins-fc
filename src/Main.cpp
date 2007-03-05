//
// => basic port to Audacious <=
//
// AMIGA Future Composer music player plugin for XMMS
// Copyright (C) 2000 Michael Schwendt <mschwendt@users.sf.net>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

extern "C"
{
#include <audacious/plugin.h>
#include <audacious/util.h>
#include <audacious/vfs.h>
}

#ifdef FC_HAVE_OLD_CPP_HEADERS
#include <string.h>
#else
#include <cstring>
using namespace std;
#endif

#include <pthread.h>

#include "Config.h"
#ifdef FC_HAVE_NOTHROW
#include <new>
#endif

#include "FC.h"
#include "MyTypes.h"

// Ugly due to the old code that comes with my FC reference
// implementation.
extern void mixerFillBuffer( void*, udword );
extern void mixerInit(udword,int,int,uword);
// ---

struct audioFormat
{
    AFormat xmmsAFormat;
    gint bits, freq, channels;
    uword zeroSample;
};

static audioFormat myFormat;

extern "C" 
{
#include "about.h"
#include "configure.h"

static void ip_init(void);
static gint ip_is_valid_file_vfs(gchar *fileName, VFSFile *fd);
static void ip_play_file(InputPlayback *playback);
static void ip_stop(InputPlayback *playback);
static void ip_pause(InputPlayback *playback, gshort p);
static void ip_seek(InputPlayback *playback, gint t);
static gint ip_get_time(InputPlayback *playback);
static void ip_get_song_info(gchar *fileName, gchar **title, gint *length);

static pthread_t decode_thread;

InputPlugin iplugin =
{
    NULL,
    NULL,
    "Future Composer Player",
    ip_init,
    fc_ip_about,
    fc_ip_configure,
    NULL,
    NULL,
    ip_play_file,
    ip_stop,
    ip_pause,
    ip_seek,
    NULL,
    ip_get_time,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ip_get_song_info,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    // 1.3.0
    ip_is_valid_file_vfs,
    NULL
};

InputPlugin *get_iplugin_info(void)
{
    return &iplugin;
}

static const udword extraFileBufLen = 8+1;  // see FC.cpp

static ubyte* fileBuf = 0;
static glong fileLen = 0;

static int sampleBufSize = 0;
static ubyte* sampleBuf = 0;

static bool playing = false;
static bool paused = true;
static int jumpToTime = -1;

static void deleteFileBuf()
{
    if (fileBuf != 0)
    {
        delete[] fileBuf;
    }
    fileBuf = 0;
    fileLen = 0;
}
    
static void deleteSampleBuf()
{
    if (sampleBuf != 0)
    {
        delete[] sampleBuf;
        sampleBuf = 0;
        sampleBufSize = 0;
    }
}

static bool loadFile(char* fileName)
{
    deleteFileBuf();
    
    VFSFile *fd = vfs_fopen(fileName,"rb");
    if (!fd) {
        return false;
    }
    if ( vfs_fseek(fd,0,SEEK_END)!=0 ) return false;
    fileLen = vfs_ftell(fd);
    if ( vfs_fseek(fd,0,SEEK_SET)!=0 ) return false;

#ifdef FC_HAVE_NOTHROW
    if ((fileBuf = new(std::nothrow) ubyte[fileLen+extraFileBufLen]) == 0)
#else
    if ((fileBuf = new ubyte[fileLen+extraFileBufLen]) == 0)
#endif
    {
        fileLen = 0;
        return false;
    }
    if ( fileLen != vfs_fread((char*)fileBuf,1,fileLen,fd) )
    {
        deleteFileBuf();
        return false;
    }
    vfs_fclose(fd);
    return true;
}

// Microsecs to sleep when audio driver does not accept new
// sample buffer contents.
static int sleepVal;

static void *play_loop(void *arg)
{
    bool stop = false;
    while ( playing )
    {
        if ( paused && stop )
        {
            memset(sampleBuf,0,sampleBufSize);
        }
        if ( jumpToTime != -1 )
        {
            iplugin.output->flush(jumpToTime);
            FC_init(fileBuf,fileLen,0,0);
            while (jumpToTime > 0)
            {
                FC_play();
                jumpToTime -= 20;
            };
            jumpToTime = -1;
            stop = false;
        }
        if ( !stop )
        {
            mixerFillBuffer(sampleBuf,sampleBufSize);
            iplugin.add_vis_pcm(iplugin.output->written_time(),
                                myFormat.xmmsAFormat, myFormat.channels,
                                sampleBufSize, sampleBuf);
            while ( iplugin.output->buffer_free() < sampleBufSize && playing )
                xmms_usleep( sleepVal );
            if ( playing && jumpToTime<0 )
                iplugin.output->write_audio(sampleBuf,sampleBufSize);
            if ( FC_songEnd && jumpToTime<0 )
            {
                iplugin.output->buffer_free();
                iplugin.output->buffer_free();
                while ( iplugin.output->buffer_playing() && playing && jumpToTime<0 )
                {
                    xmms_usleep(10000);
                }
                stop = true;
            }
        }
        else
        {
            xmms_usleep( 10000 );
        }
    }
    pthread_exit(NULL);
}

static void ip_init(void)
{
    playing = false;
    fileBuf = 0;
    fileLen = 0;
    sampleBuf = 0;
    sampleBufSize = 0;
    
    fc_ip_load_config();
}

static gint ip_is_valid_file_vfs(gchar *fileName, VFSFile *fd)
{
    ubyte magicBuf[5];
    if ( 5 != vfs_fread(magicBuf,1,5,fd) ) {
        return false;
    }
    
    // See FC.cpp
    // Check for "SMOD" ID (Future Composer 1.0 to 1.3).
    bool isSMOD = (magicBuf[0]==0x53 && magicBuf[1]==0x4D &&
                   magicBuf[2]==0x4F && magicBuf[3]==0x44 &&
                   magicBuf[4]==0x00);
    
    // Check for "FC14" ID (Future Composer 1.4).
    bool isFC14 = (magicBuf[0]==0x46 && magicBuf[1]==0x43 &&
                   magicBuf[2]==0x31 && magicBuf[3]==0x34 &&
                   magicBuf[4]==0x00);
    
    return ((isSMOD|isFC14) ? 1 : 0);
}

static void ip_play_file(InputPlayback *playback)
{
    playing = false;
    paused = true;
    jumpToTime = -1;
    
    bool haveModule = false;

    if ( loadFile(playback->filename) )
    {
        haveModule = FC_init(fileBuf,fileLen,0,0);
    }
    else {
        return;
    }
    myFormat.freq = fc_myConfig.frequency;
    myFormat.channels = fc_myConfig.channels;
    if (fc_myConfig.precision == 8)
    {
        myFormat.xmmsAFormat = FMT_U8;
        myFormat.bits = 8;
        myFormat.zeroSample = 0x80;
    }
    else
    {
#ifdef WORDS_BIGENDIAN
        myFormat.xmmsAFormat = FMT_S16_BE;
#else
        myFormat.xmmsAFormat = FMT_S16_LE;
#endif
        myFormat.bits = 16;
        myFormat.zeroSample = 0x0000;
    }

    bool audioDriverOK = false;
    bool haveSampleBuf = false;
    
    if (myFormat.freq>0 && myFormat.channels>0)
    {
        audioDriverOK = (iplugin.output->open_audio(myFormat.xmmsAFormat,
                                                    myFormat.freq,
                                                    myFormat.channels) != 0);
    }

    if ( !audioDriverOK )
    {
        // Try some audio configurations regardless of whether the
        // output plugin might perform format conversion and/or
        // downsampling.
        audioFormat formatList[] = 
        {
#ifdef WORDS_BIGENDIAN
            { FMT_S16_BE, 16, 44100, 1, 0x0000 },
#else
            { FMT_S16_LE, 16, 44100, 1, 0x0000 },
#endif
            { FMT_U8,      8, 44100, 1,   0x80 },
            { FMT_U8,      8, 22050, 1,   0x80 },
            { FMT_U8,      0,     0, 0,      0 }
        };
    
        int i = 0;
        do
        {
            myFormat = formatList[i];
            
            if (iplugin.output->open_audio(myFormat.xmmsAFormat,
                                           myFormat.freq,
                                           myFormat.channels) != 0)
            {
                audioDriverOK = true;
                break;
            }
        }
        while (formatList[++i].bits != 0);
    }

    if ( audioDriverOK )
    {
        sampleBufSize = 512*(myFormat.bits/8)*myFormat.channels;
#ifdef FC_HAVE_NOTHROW
        haveSampleBuf = ((sampleBuf = new(std::nothrow) ubyte[sampleBufSize]) != 0);
#else
        haveSampleBuf = ((sampleBuf = new ubyte[sampleBufSize]) != 0);
#endif
        mixerInit(myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample);
    }

    if ( haveSampleBuf && haveModule )
    {
        int msecSongLen = 0;
        do
        {
            FC_play();
            msecSongLen += 20;
        }
        while ( !FC_songEnd );
        FC_init(fileBuf,fileLen,0,0);
        mixerInit(myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample);
    
        gint bitsPerSec = myFormat.freq * myFormat.channels * myFormat.bits;
        gchar* title;
        gchar* pathSepPos = strrchr( playback->filename, '/' );
        if ( pathSepPos!=0 && pathSepPos[1]!=0 )  // found file name
            title = pathSepPos+1;
        else
            title = playback->filename;
        iplugin.set_info( title, msecSongLen, bitsPerSec, myFormat.freq, myFormat.channels );

        int usecPerBuf = (sampleBufSize * 1000000) / (myFormat.freq * myFormat.channels * (myFormat.bits/8));
        sleepVal = usecPerBuf;
        
        paused = false;
        playing = true;
        
        pthread_create(&decode_thread, NULL, play_loop, NULL);
    }
    else
    {
        deleteSampleBuf();
        deleteFileBuf();
    }
}
    
static void ip_stop(InputPlayback *playback)
{
    if ( fileBuf!=0 && playing )
    {
        playing = false;
        paused = true;
        pthread_join(decode_thread, NULL);
        iplugin.output->close_audio();
        
        deleteSampleBuf();
        deleteFileBuf();
    }
}

static void ip_pause(InputPlayback *playback, gshort p)
{
    iplugin.output->pause(p);
}

static void ip_seek(InputPlayback *playback, gint secs)
{
    jumpToTime = secs * 1000;
    
    while (jumpToTime != -1)
        xmms_usleep(10000);
}

static gint ip_get_time(InputPlayback *playback)
{
    if (iplugin.output)
    {
        if ( playing && FC_songEnd && !iplugin.output->buffer_playing() )
            return -1;  // stop and next file
        else if ( playing )
            return iplugin.output->output_time();
        else
            return -1;
    }
    else
        return -1;
}

static void ip_get_song_info(gchar *fileName, gchar **title, gint *length)
{
    gchar* songTitle;
    gchar* pathSepPos = strrchr( fileName, '/' );
    if ( pathSepPos!=0 && pathSepPos[1]!=0 )  // found file name
        songTitle = pathSepPos+1;
    else
        songTitle = fileName;
    *title = g_strdup( songTitle );
    *length = -1;
}

}  // extern "C"
