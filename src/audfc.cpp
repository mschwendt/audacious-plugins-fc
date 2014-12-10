//
// AMIGA Future Composer music player plugin for Audacious
// Copyright (C) 2000,2014 Michael Schwendt
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

#include <libaudcore/plugin.h>
#include <libaudcore/vfs.h>
#include <fc14audiodecoder.h>
#include <cstdlib>

#if _AUD_PLUGIN_VERSION < 46
#error "At least Audacious 3.6-alpha1 is required."
#endif

#include "config.h"
#include "configure.h"
#include "audfc.h"

struct audioFormat
{
    int xmmsAFormat;
    int bits, freq, channels;
    int zeroSample;
};

EXPORT AudFC aud_plugin_instance;

const char AudFC::about[] =
   "Created by Michael Schwendt.\n\n"
   "http://xmms-fc.sourceforge.net\n"
   "Much room for improvement.\nHave a go if you like to.\n";

const char *const AudFC::exts[] = {
    "fc",
    "fc13",
    "fc14",
    nullptr
};

const char *const AudFC::defaults[] = {
    "frequency", "44100",
    "precision", "8",
    "channels", "1",
    nullptr
};

bool AudFC::init(void) {
    fc_ip_load_config();

    return true;
}

bool AudFC::is_our_file(const char *fileName, VFSFile &fd) {
    void *dec;
    unsigned char magicBuf[5];
    int ret;

    if ( 5 != fd.fread(magicBuf,1,5) ) {
        return 1;
    }
    dec = fc14dec_new();
    ret = fc14dec_detect(dec,magicBuf,5);
    fc14dec_delete(dec);
    return ret;
}

bool AudFC::play(const char *filename, VFSFile &fd) {
    void *decoder = nullptr;
    void *sampleBuf = nullptr;
    size_t sampleBufSize;
    void *fileBuf = nullptr;
    size_t fileLen;
    bool haveModule = false;
    bool audioDriverOK = false;
    bool haveSampleBuf = false;
    struct audioFormat myFormat;

    if ( fd.fseek(0,VFS_SEEK_END)!=0 ) {
        return false;
    }
    fileLen = fd.ftell();
    if ( fd.fseek(0,VFS_SEEK_SET)!=0 ) {
        return false;
    }
    fileBuf = malloc(fileLen);
    if ( !fileBuf ) {
        return false;
    }
    if ( fileLen != fd.fread((char*)fileBuf,1,fileLen) ) {
        free(fileBuf);
        return false;
    }
    decoder = fc14dec_new();
    haveModule = fc14dec_init(decoder,fileBuf,fileLen);
    free(fileBuf);
    if ( !haveModule ) {
        fc14dec_delete(decoder);
        return false;
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
        open_audio(myFormat.xmmsAFormat,
                   myFormat.freq,
                   myFormat.channels);
    }
    sampleBufSize = 512*(myFormat.bits/8)*myFormat.channels;
    sampleBuf = malloc(sampleBufSize);
    haveSampleBuf = (sampleBuf != nullptr);
    fc14dec_mixer_init(decoder,myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample);

    if ( haveSampleBuf && haveModule ) {
        int msecSongLen = fc14dec_duration(decoder);

        Tuple t;
        t.set_filename(filename);
        t.set_int(Tuple::Length,msecSongLen);
        t.set_str(Tuple::Quality,"sequenced");
        set_playback_tuple( std::move(t) );

        while ( !check_stop() ) {
            int jumpToTime = check_seek();
            if ( jumpToTime != -1 ) {
                fc14dec_seek(decoder,jumpToTime);
            }

            fc14dec_buffer_fill(decoder,sampleBuf,sampleBufSize);
            write_audio(sampleBuf,sampleBufSize);
            if ( fc14dec_song_end(decoder) ) {
                break;
            }
        }
    }

    free(sampleBuf);
    fc14dec_delete(decoder);
    return true;
}
    
Tuple AudFC::read_tuple(const char *filename, VFSFile &fd) {
    void *decoder = nullptr;
    void *fileBuf = nullptr;
    size_t fileLen;

    if ( fd.fseek(0,VFS_SEEK_END)!=0 ) {
        return Tuple();
    }
    fileLen = fd.ftell();
    if ( fd.fseek(0,VFS_SEEK_SET)!=0 ) {
        return Tuple();
    }
    fileBuf = malloc(fileLen);
    if ( !fileBuf ) {
        return Tuple();
    }
    if ( fileLen != fd.fread((char*)fileBuf,1,fileLen) ) {
        free(fileBuf);
        return Tuple();
    }
    decoder = fc14dec_new();
    Tuple t;
    if (fc14dec_init(decoder,fileBuf,fileLen)) {
        t.set_filename(filename);
        t.set_int(Tuple::Length,fc14dec_duration(decoder));
        t.set_str(Tuple::Quality,"sequenced");
    }
    free(fileBuf);
    fc14dec_delete(decoder);
    return t;
}
