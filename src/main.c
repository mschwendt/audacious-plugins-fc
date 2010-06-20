//
// => basic port to Audacious 2 <=
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

#include <audacious/plugin.h>
#include <glib.h>
#include <fc14audiodecoder.h>

#include "config.h"
#include "configure.h"

struct audioFormat
{
    AFormat xmmsAFormat;
    gint bits, freq, channels;
    gint zeroSample;
};

static GMutex *seek_mutex;
static GCond *seek_cond;
static gint jumpToTime = -1;

void ip_init(void) {
    jumpToTime = -1;
    seek_mutex = g_mutex_new();
    seek_cond = g_cond_new();
    
    fc_ip_load_config();
}

void ip_cleanup(void) {
    g_cond_free(seek_cond);
    g_mutex_free(seek_mutex);
}

gint ip_is_valid_file_vfs(const gchar *fileName, VFSFile *fd) {
    void *dec;
    unsigned char magicBuf[5];
    int ret;

    if ( 5 != aud_vfs_fread(magicBuf,1,5,fd) ) {
        return 1;
    }
    dec = fc14dec_new();
    ret = fc14dec_detect(dec,magicBuf,5);
    fc14dec_delete(dec);
    return ret;
}

void ip_play_file(InputPlayback *playback) {
    void *decoder = NULL;
    gpointer sampleBuf = NULL;
    size_t sampleBufSize;
    VFSFile *fd;
    gpointer fileBuf = NULL;
    size_t fileLen;
    gboolean haveModule = FALSE;
    gboolean audioDriverOK = FALSE;
    gboolean haveSampleBuf = FALSE;
    struct audioFormat myFormat;

    playback->playing = FALSE;
    jumpToTime = -1;
    decoder = fc14dec_new();

    fd = aud_vfs_fopen(playback->filename,"rb");
    if (!fd) {
        goto PLAY_FAILURE_1;
    }
    if ( aud_vfs_fseek(fd,0,SEEK_END)!=0 ) {
        aud_vfs_fclose(fd);
        goto PLAY_FAILURE_1;
    }
    fileLen = aud_vfs_ftell(fd);
    if ( aud_vfs_fseek(fd,0,SEEK_SET)!=0 ) {
        aud_vfs_fclose(fd);
        goto PLAY_FAILURE_1;
    }
    fileBuf = g_malloc(fileLen);
    if ( !fileBuf ) {
        aud_vfs_fclose(fd);
        goto PLAY_FAILURE_1;
    }
    if ( fileLen != aud_vfs_fread((char*)fileBuf,1,fileLen,fd) ) {
        aud_vfs_fclose(fd);
        g_free(fileBuf);
        goto PLAY_FAILURE_1;
    }
    aud_vfs_fclose(fd);
    haveModule = fc14dec_init(decoder,fileBuf,fileLen);
    g_free(fileBuf);
    if ( !haveModule ) {
        goto PLAY_FAILURE_1;
    }

    myFormat.freq = fc_myConfig.frequency;
    myFormat.channels = fc_myConfig.channels;
    if (fc_myConfig.precision == 8) {
        myFormat.xmmsAFormat = FMT_U8;
        myFormat.bits = 8;
        myFormat.zeroSample = 0x80;
    }
    else {
#ifdef WORDS_BIGENDIAN
        myFormat.xmmsAFormat = FMT_S16_BE;
#else
        myFormat.xmmsAFormat = FMT_S16_LE;
#endif
        myFormat.bits = 16;
        myFormat.zeroSample = 0x0000;
    }
    if (myFormat.freq>0 && myFormat.channels>0) {
        audioDriverOK = (playback->output->open_audio(myFormat.xmmsAFormat,
                                                      myFormat.freq,
                                                      myFormat.channels) != 0);
    }
    if ( !audioDriverOK ) {
        // Try some audio configurations regardless of whether the
        // output plugin might perform format conversion and/or
        // downsampling.
        struct audioFormat formatList[] = {
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
        do {
            myFormat = formatList[i];
            if (playback->output->open_audio(myFormat.xmmsAFormat,
                                             myFormat.freq,
                                             myFormat.channels) != 0)
            {
                audioDriverOK = TRUE;
                break;
            }
        }
        while (formatList[++i].bits != 0);
    }
    if ( audioDriverOK ) {
        sampleBufSize = 512*(myFormat.bits/8)*myFormat.channels;
        sampleBuf = g_malloc(sampleBufSize);
        haveSampleBuf = (sampleBuf != NULL);
        fc14dec_mixer_init(decoder,myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample);
    }

    if ( haveSampleBuf && haveModule ) {
        int msecSongLen = fc14dec_duration(decoder);

        Tuple *t = aud_tuple_new_from_filename( playback->filename );
        aud_tuple_associate_int(t, FIELD_LENGTH, NULL, msecSongLen);
        aud_tuple_associate_string(t, FIELD_QUALITY, NULL, "sequenced");
        playback->set_tuple( playback, t );

        gint bitsPerSec = myFormat.freq * myFormat.channels * myFormat.bits;
        playback->set_params( playback, NULL, 0, bitsPerSec, myFormat.freq, myFormat.channels );
        
        playback->playing = TRUE;
        playback->set_pb_ready(playback);

        while ( playback->playing ) {
            fc14dec_buffer_fill(decoder,sampleBuf,sampleBufSize);
            if ( playback->playing && jumpToTime<0 ) {
#ifdef __AUDACIOUS_PLUGIN_API__ >= 13
                playback->output->write_audio(sampleBuf,sampleBufSize);
#else
                playback->pass_audio(playback,myFormat.xmmsAFormat,myFormat.channels,sampleBufSize,sampleBuf,NULL);
#endif
            }
            if ( fc14dec_song_end(decoder) && jumpToTime<0 ) {
                playback->eof = TRUE;
                playback->playing = FALSE;
            }

            g_mutex_lock(seek_mutex);
            if ( jumpToTime != -1 ) {
                fc14dec_seek(decoder,jumpToTime);
                playback->output->flush(jumpToTime);
                jumpToTime = -1;
                g_cond_signal(seek_cond);
            }
            g_mutex_unlock(seek_mutex);
        }
        playback->playing = FALSE;
        playback->output->close_audio();
    }

 PLAY_FAILURE_2:
    g_free(sampleBuf);
 PLAY_FAILURE_1:
    fc14dec_delete(decoder);
}
    
void ip_stop(InputPlayback *playback) {
    playback->playing = FALSE;
}

void ip_pause(InputPlayback *playback, gshort p) {
    playback->output->pause(p);
}

void ip_mseek(InputPlayback *playback, gulong msec) {
    g_mutex_lock(seek_mutex);
    jumpToTime = msec;
    g_cond_wait(seek_cond, seek_mutex);
    g_mutex_unlock(seek_mutex);
}

void ip_seek(InputPlayback *playback, gint secs) {
    gulong msec = secs * 1000;
    ip_mseek(playback, msec);
}

Tuple* ip_get_song_tuple(const gchar *filename) {
    Tuple *t = aud_tuple_new_from_filename(filename);

    /* delay length detection to start of playback */
    aud_tuple_associate_int(t, FIELD_LENGTH, NULL, -1);
    aud_tuple_associate_string(t, FIELD_QUALITY, NULL, "sequenced");
    /* aud_tuple_associate_string(ti, FIELD_TITLE, NULL, tmp); */

    return t;
}
