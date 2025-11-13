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
#include <tfmxaudiodecoder.h>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <cstring>

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
    "TFMX (AMIGA)\n"
    "Future Composer (AMIGA)\n"
    "Hippel TFMX (AMIGA)\n"
    "\n"
    "File name extensions:\n"
    ".tfmx, .tfx, .tfm, .mdat\n"
    ".fc, .fc13, .fc14, .fc3, .fc4, .smod\n"
    ".hip, .hipc, .hip7, .mcmd\n"
    "\n"
    "Plugin version: " VERSION "\n"
    "Created by Michael Schwendt\n";

const char *const AudFC::exts[] = {
    "tfmx", "tfx", "tfm", "mdat",
    "fc", "fc13", "fc14", "fc3", "fc4", "smod",
    "hip", "hipc", "hip7", "mcmd",
    nullptr
};

const char *const AudFC::defaults[] = {
    "frequency", "44100",
    "precision", "16",
    "channels", "2",
    "panning", "75",
    "ignoreshorts", "TRUE",
    "endshorts", "FALSE",
    "maxsecs", "10",
    nullptr
};

bool AudFC::init(void) {
    fc_ip_load_config();

    return true;
}

bool AudFC::is_our_file(const char *fileName, VFSFile &fd) {
    void *dec;
    const int minSize = 0xb80;  // preferred minimal probe buffer size
    unsigned char probeBuf[minSize];
    int ret;
    std::string path, ext;
    parse_uri(fileName,path,ext);

    int64_t readSize = fd.fread(probeBuf,1,minSize);
    if (readSize<minSize && !fd.feof()) {
        return false;
    }
    dec = tfmxdec_new();
    tfmxdec_set_path(dec,path.c_str());
    ret = tfmxdec_detect(dec,probeBuf,readSize);
    tfmxdec_delete(dec);
    return ret;
}

bool AudFC::play(const char *uri, VFSFile &fd) {
    void *decoder = nullptr;
    void *sampleBuf = nullptr;
    size_t sampleBufSize;
    bool haveModule = false;
    //bool audioDriverOK = false;
    bool haveSampleBuf = false;
    struct audioFormat myFormat;
    int songNumber = -1;
    std::string path, ext;

    songNumber = parse_uri(uri,path,ext);

    Index<char> fileBuf = fd.read_all();
    decoder = tfmxdec_new();
    tfmxdec_set_path(decoder,path.c_str());
    tfmxdec_end_shorts(decoder,fc_myConfig.endshorts,fc_myConfig.maxsecs);
    haveModule = tfmxdec_init(decoder,fileBuf.begin(),fileBuf.len(),songNumber-1);
    if ( !haveModule ) {
        tfmxdec_delete(decoder);
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
    tfmxdec_mixer_init(decoder,myFormat.freq,myFormat.bits,myFormat.channels,myFormat.zeroSample,fc_myConfig.panning);

    if ( haveSampleBuf && haveModule ) {
        Tuple t;
        t.set_filename(uri);
        fill_tuple(t,decoder);
        if (songNumber > 0) {
            //t.set_int(Tuple::Subtune,songNumber);
            t.set_int(Tuple::Track,songNumber);
        }
        set_playback_tuple( std::move(t) );

        while ( !check_stop() ) {
            int jumpToTime = check_seek();
            if ( jumpToTime != -1 ) {
                tfmxdec_seek(decoder,jumpToTime);
            }

            tfmxdec_buffer_fill(decoder,sampleBuf,sampleBufSize);
            write_audio(sampleBuf,sampleBufSize);
            if ( tfmxdec_song_end(decoder) ) {
                break;
            }
        }
    }

    free(sampleBuf);
    tfmxdec_delete(decoder);
    return true;
}
    
bool AudFC::read_tag(const char *uri, VFSFile &fd, Tuple &t, Index<char> *image) {
    void *decoder = nullptr;
    std::string path, ext;
    int songArg = parse_uri(uri,path,ext);

    int songNumber = t.get_int(Tuple::Subtune);

    Index<char> fileBuf = fd.read_all();
    decoder = tfmxdec_new();
    tfmxdec_set_path(decoder,path.c_str());
    if (tfmxdec_init(decoder,fileBuf.begin(),fileBuf.len(),songNumber-1)) {
        fill_tuple(t,decoder);
        t.set_filename(uri);
        int songs = tfmxdec_songs(decoder);
        // Populate individual track/song tuples.
        if ( songNumber>0 || songs==1 ) {
            if (songs > 1) {
                t.set_int(Tuple::Subtune,songNumber);
                t.set_int(Tuple::Track,songNumber);
            }
        }
        // Populate index of sub-tracks/sub-songs.
        else {
            Index<short> subtunes;
            int songsAccepted = 0;
            for (int s=0; s<songs; s++) {
                if (tfmxdec_reinit(decoder,s) ) {
                    int dur = tfmxdec_duration(decoder);
                    if (!fc_myConfig.ignoreshorts || (dur/1000) >= fc_myConfig.maxsecs) {
                        subtunes.append(s+1);
                        songsAccepted++;
                    }
                }
            }
            t.set_subtunes(subtunes.len(),subtunes.begin());
            t.set_int(Tuple::NumSubtunes,songsAccepted);
        }
    }
    tfmxdec_delete(decoder);
    return true;
}

int AudFC::parse_uri(const char *uri, std::string &path, std::string &ext) {
    int subSong;
    const char *sub, *tmpExt;

    // Audacious doesn't strip the appended song number arg like "?1".
    uri_parse(uri, nullptr, &tmpExt, &sub, &subSong);
#ifdef __MINGW32__
    std::string p = std::string(uri_to_filename(uri));
    std::replace(p.begin(), p.end(), '\\', '/');
    const char *tmpPath = p.c_str();
#else
    const char *tmpPath = uri_to_filename(uri);
#endif
    path = std::string(tmpPath, strlen(tmpPath) - strlen(sub));
    ext = std::string(tmpExt, strlen(tmpExt) - strlen(sub));

    return strlen(sub) > 0 ? subSong : -1;
}

void AudFC::fill_tuple(Tuple &t, void *decoder) {
    const char *s = tfmxdec_get_artist(decoder);
    if ( s!=0 && strlen(s)>0 ) {
        t.set_str(Tuple::Artist,s);
    }
    s = tfmxdec_get_title(decoder);
    if ( s!=0 && strlen(s)>0 ) {
        t.set_str(Tuple::Title,s);
    }
    else {
        s = tfmxdec_get_name(decoder);
        if ( s!=0 && strlen(s)>0 ) {
            t.set_str(Tuple::Title,s);
        }
    }
    s = tfmxdec_get_game(decoder);
    if ( s!=0 && strlen(s)>0 ) {
        t.set_str(Tuple::Album,s);
    }
    t.set_str(Tuple::Codec,tfmxdec_format_name(decoder));
    t.set_str(Tuple::Quality,"sequenced");
    t.set_int(Tuple::Length,tfmxdec_duration(decoder));
}
