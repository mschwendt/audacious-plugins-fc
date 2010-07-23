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

#if __AUDACIOUS_PLUGIN_API__ < 16
#error "At least Audacious 2.4 beta1 is required."
#endif

#include "config.h"
#include "configure.h"

struct audioFormat
{
    gint xmmsAFormat;
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

    if ( 5 != vfs_fread(magicBuf,1,5,fd) ) {
        return 1;
    }
    dec = fc14dec_new();
    ret = fc14dec_detect(dec,magicBuf,5);
    fc14dec_delete(dec);
    return ret;
}

gboolean ip_play(InputPlayback *playback, const gchar *filename, VFSFile *fd,
                 gint start_time, gint stop_time, gboolean pause) {
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

    playback->playing = FALSE;
    jumpToTime = (start_time > 0) ? start_time : -1;

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
        if (pause) {
            playback->output->pause(TRUE);
        }
        sampleBufSize = 512*(myFormat.bits/8)*myFormat.channels;
        sampleBuf = g_malloc(sampleBufSize);
        haveSampleBuf = (sampleBuf != NULL);
        fc14dec_mixer_init(decoder,myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample);
    }

    if ( haveSampleBuf && haveModule ) {
        int msecSongLen = fc14dec_duration(decoder);

        Tuple *t = tuple_new_from_filename( playback->filename );
        tuple_associate_int(t, FIELD_LENGTH, NULL, msecSongLen);
        tuple_associate_string(t, FIELD_QUALITY, NULL, "sequenced");
        playback->set_tuple( playback, t );

        /* bitrate => 4*1000 will be displayed as "4 CHANNELS" */
        playback->set_params( playback, NULL, 0, 1000*4, myFormat.freq, myFormat.channels );
        
        playback->playing = TRUE;
        playback->set_pb_ready(playback);

        while ( playback->playing ) {
            if (stop_time >= 0 && playback->output->written_time () >= stop_time) {
                goto DRAIN;
            }
            g_mutex_lock(seek_mutex);
            if ( jumpToTime != -1 ) {
                fc14dec_seek(decoder,jumpToTime);
                playback->output->flush(jumpToTime);
                jumpToTime = -1;
                g_cond_signal(seek_cond);
            }
            g_mutex_unlock(seek_mutex);

            fc14dec_buffer_fill(decoder,sampleBuf,sampleBufSize);
            if ( playback->playing && jumpToTime<0 ) {
                playback->output->write_audio(sampleBuf,sampleBufSize);
            }
            if ( fc14dec_song_end(decoder) && jumpToTime<0 ) {
                playback->eof = TRUE;
                playback->playing = FALSE;
 DRAIN:
                while (playback->output->buffer_playing() && playback->playing) {
                    g_usleep(20000);
                }
                break;
            }
        }
    }
 CLEANUP:
    g_mutex_lock(seek_mutex);
    playback->playing = FALSE;
    g_cond_signal(seek_cond);  /* wake up any waiting request */
    g_mutex_unlock(seek_mutex);

    playback->output->close_audio();
    g_free(sampleBuf);
    fc14dec_delete(decoder);
    return FALSE;
}
    
void ip_stop(InputPlayback *playback) {
    g_mutex_lock(seek_mutex);
    if (playback->playing) {
        playback->playing = FALSE;
        playback->output->abort_write();
        g_cond_signal(seek_cond);
    }
    g_mutex_unlock(seek_mutex);
}

void ip_pause(InputPlayback *playback, gshort p) {
    g_mutex_lock(seek_mutex);
    if (playback->playing) {
        playback->output->pause(p);
    }
    g_mutex_unlock(seek_mutex);
}

void ip_mseek(InputPlayback *playback, gulong msec) {
    g_mutex_lock(seek_mutex);
    if (playback->playing) {
        jumpToTime = msec;
        playback->output->abort_write();
        g_cond_signal(seek_cond);
        g_cond_wait(seek_cond, seek_mutex);
    }
    g_mutex_unlock(seek_mutex);
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
        tuple_associate_int(t, FIELD_LENGTH, NULL, fc14dec_duration(decoder));
        tuple_associate_string(t, FIELD_QUALITY, NULL, "sequenced");
    }
    else {
        t = NULL;
    }
    g_free(fileBuf);
    fc14dec_delete(decoder);
    return t;
}
