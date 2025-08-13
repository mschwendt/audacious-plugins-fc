//
// AMIGA Future Composer and Hippel TFMX music plugin for Audacious
// Copyright (C) Michael Schwendt
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
#include <libaudcore/audstrings.h>
#include <libaudcore/vfs.h>
#include <fc14audiodecoder.h>
#include <cstdlib>

#if _AUD_PLUGIN_VERSION < 48
#error "At least Audacious 3.8 is required."
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
    "Decoder for:\n"
    "\n"
    "Future Composer (AMIGA)\n"
    "Hippel TFMX (AMIGA)\n"
    "\n"
    "File name extensions:\n"
    ".fc, .fc13, .fc14, .fc3, .fc4, .smod\n"
    ".hip, .hipc, .hip7, .mcmd\n"
    "\n"
    "Plugin version: " VERSION "\n"
    "Created by Michael Schwendt\n";

const char *const AudFC::exts[] = {
    "fc",
    "fc13",
    "fc14",
    "fc3",
    "fc4",
    "smod",
    "hip",
    "hipc",
    "hip7",
    "mcmd",
    nullptr
};

const char *const AudFC::defaults[] = {
    "frequency", "44100",
    "precision", "8",
    "channels", "1",
    "panning", "75",
    nullptr
};

bool AudFC::init(void) {
    fc_ip_load_config();

    return true;
}

bool AudFC::is_our_file(const char *fileName, VFSFile &fd) {
    void *dec;
    const int minSize = 2560;
    unsigned char magicBuf[minSize];
    int ret;

    if ( minSize != fd.fread(magicBuf,1,minSize) ) {
        return false;
    }
    dec = fc14dec_new();
    ret = fc14dec_detect(dec,magicBuf,minSize);
    fc14dec_delete(dec);
    return ret;
}

bool AudFC::play(const char *filename, VFSFile &fd) {
    void *decoder = nullptr;
    void *sampleBuf = nullptr;
    size_t sampleBufSize;
    bool haveModule = false;
    bool audioDriverOK = false;
    bool haveSampleBuf = false;
    struct audioFormat myFormat;
    int songNumber = -1;

    uri_parse(filename,nullptr,nullptr,nullptr,&songNumber);

    Index<char> fileBuf = fd.read_all();
    decoder = fc14dec_new();
    haveModule = fc14dec_init(decoder,fileBuf.begin(),fileBuf.len(),songNumber-1);
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
    fc14dec_mixer_init(decoder,myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample,fc_myConfig.panning);

    if ( haveSampleBuf && haveModule ) {
        Tuple t;
        t.set_filename(filename);
        t.set_str(Tuple::Codec,fc14dec_format_name(decoder));
        t.set_int(Tuple::Length,fc14dec_duration(decoder));
        t.set_str(Tuple::Quality,"sequenced");
        if (songNumber > 0) {
            //t.set_int(Tuple::Subtune,songNumber);
            t.set_int(Tuple::Track,songNumber);
        }
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
    
bool AudFC::read_tag(const char *filename, VFSFile &fd, Tuple &t, Index<char> *image) {
    void *decoder = nullptr;

    int songNumber = t.get_int(Tuple::Subtune);

    Index<char> fileBuf = fd.read_all();
    decoder = fc14dec_new();
    if (fc14dec_init(decoder,fileBuf.begin(),fileBuf.len(),songNumber-1)) {
        t.set_filename(filename);
        t.set_str(Tuple::Codec,fc14dec_format_name(decoder));
        t.set_int(Tuple::Length,fc14dec_duration(decoder));
        t.set_str(Tuple::Quality,"sequenced");

        int songs = fc14dec_songs(decoder);
        if (songs > 1) {
            t.set_int(Tuple::NumSubtunes,songs);
            if (songNumber > 0) {
                t.set_int(Tuple::Subtune,songNumber);
                t.set_int(Tuple::Track,songNumber);
            }
            Index<short> subtunes;
            for (int s=0; s<songs; s++)
                subtunes.append(s+1);
            t.set_subtunes(subtunes.len(),subtunes.begin());
        }
    }
    fc14dec_delete(decoder);
    return true;
}
