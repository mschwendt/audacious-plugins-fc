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
#include <audacious/input.h>
#include <glib.h>
#include <fc14audiodecoder.h>
#include <stdio.h>

#if _AUD_PLUGIN_VERSION < 45
#error "At least Audacious 3.5-devel is required."
#endif

#include "config.h"
#include "configure.h"

struct audioFormat
{
    gint xmmsAFormat;
    gint bits, freq, channels;
    gint zeroSample;
};

gboolean ip_init(void) {
    fc_ip_load_config();

    return TRUE;
}

gint ip_is_valid_file_vfs(const gchar *fileName, VFSFile *fd) {
    void *dec;
    unsigned char magicBuf[5];
    int ret;

    if ( 5 != vfs_fread(magicBuf,1,5,fd) ) {
        return 1;
    }
    dec = fc14dec_new();
    ret = fc14dec_detect(dec,magicBuf,5);
    fc14dec_delete(dec);
    return ret;
}

gboolean ip_play(const gchar *filename, VFSFile *fd) {
    void *decoder = NULL;
    gpointer sampleBuf = NULL;
    size_t sampleBufSize;
    gpointer fileBuf = NULL;
    size_t fileLen;
    gboolean haveModule = FALSE;
    gboolean audioDriverOK = FALSE;
    gboolean haveSampleBuf = FALSE;
    struct audioFormat myFormat;

    if (fd == NULL) {
        return FALSE;
    }

    if ( vfs_fseek(fd,0,SEEK_END)!=0 ) {
        return FALSE;
    }
    fileLen = vfs_ftell(fd);
    if ( vfs_fseek(fd,0,SEEK_SET)!=0 ) {
        return FALSE;
    }
    fileBuf = g_malloc(fileLen);
    if ( !fileBuf ) {
        return FALSE;
    }
    if ( fileLen != vfs_fread((char*)fileBuf,1,fileLen,fd) ) {
        g_free(fileBuf);
        return FALSE;
    }
    decoder = fc14dec_new();
    haveModule = fc14dec_init(decoder,fileBuf,fileLen);
    g_free(fileBuf);
    if ( !haveModule ) {
        fc14dec_delete(decoder);
        return FALSE;
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
        audioDriverOK = aud_input_open_audio(myFormat.xmmsAFormat,
                                                      myFormat.freq,
                                             myFormat.channels);
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
            if (!aud_input_open_audio(myFormat.xmmsAFormat,
                                             myFormat.freq,
                                      myFormat.channels))
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

        Tuple *t = tuple_new_from_filename( filename );
        tuple_set_int(t, FIELD_LENGTH, NULL, msecSongLen);
        tuple_set_str(t, FIELD_QUALITY, NULL, "sequenced");
        aud_input_set_tuple( t );

        // TODO
        /* bitrate => 4*1000 will be displayed as "4 CHANNELS" */
        //        playback->set_params( playback, 1000*4, myFormat.freq, myFormat.channels );
        
        while ( !aud_input_check_stop() ) {
            int jumpToTime = aud_input_check_seek();
            if ( jumpToTime != -1 ) {
                fc14dec_seek(decoder,jumpToTime);
            }

            fc14dec_buffer_fill(decoder,sampleBuf,sampleBufSize);
            aud_input_write_audio(sampleBuf,sampleBufSize);
            if ( fc14dec_song_end(decoder) ) {
                break;
            }
        }
    }

    g_free(sampleBuf);
    fc14dec_delete(decoder);
    return TRUE;
}
    
Tuple *ip_probe_for_tuple(const gchar *filename, VFSFile *fd) {
    void *decoder = NULL;
    gpointer fileBuf = NULL;
    size_t fileLen;
    Tuple *t;

    if ( fd==NULL || vfs_fseek(fd,0,SEEK_END)!=0 ) {
        return NULL;
    }
    fileLen = vfs_ftell(fd);
    if ( vfs_fseek(fd,0,SEEK_SET)!=0 ) {
        return NULL;
    }
    fileBuf = g_malloc(fileLen);
    if ( !fileBuf ) {
        return NULL;
    }
    if ( fileLen != vfs_fread((char*)fileBuf,1,fileLen,fd) ) {
        g_free(fileBuf);
        return NULL;
    }
    decoder = fc14dec_new();
    if (fc14dec_init(decoder,fileBuf,fileLen)) {
        t = tuple_new_from_filename(filename);
        tuple_set_int(t, FIELD_LENGTH, NULL, fc14dec_duration(decoder));
        tuple_set_str(t, FIELD_QUALITY, NULL, "sequenced");
    }
    else {
        t = NULL;
    }
    g_free(fileBuf);
    fc14dec_delete(decoder);
    return t;
}
