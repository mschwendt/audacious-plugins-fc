#ifndef FC_DEFS_H
#define FC_DEFS_H

#include "FC.h"
#include "MyTypes.h"
#include "MyEndian.h"
#include "LamePaula.h"
#include "SmartPtr.h"

static const uword SMOD_SONGTAB_OFFSET = 0x0064;      // 100

static const uword FC14_SMPHEADERS_OFFSET = 0x0028;   // 40
static const uword FC14_WAVEHEADERS_OFFSET = 0x0064;  // 100
static const uword FC14_SONGTAB_OFFSET = 0x00b4;      // 180

static const uword TRACKTAB_ENTRY_LENGTH = 0x000d;    // 3*4+1
static const uword PATTERN_LENGTH = 0x0040;           // 32*2
static const ubyte PATTERN_BREAK = 0x49;

static const ubyte SEQ_END = 0xE1;

static const ubyte SNDMOD_LOOP = 0xE0;
static const ubyte SNDMOD_END = SEQ_END;
static const ubyte SNDMOD_SETWAVE = 0xE2;
static const ubyte SNDMOD_CHANGEWAVE = 0xE4;
static const ubyte SNDMOD_NEWVIB = 0xE3;
static const ubyte SNDMOD_SUSTAIN = 0xE8;
static const ubyte SNDMOD_NEWSEQ = 0xE7;
static const ubyte SNDMOD_SETPACKWAVE = 0xE9;
static const ubyte SNDMOD_PITCHBEND = 0xEA;

static const ubyte ENVELOPE_LOOP = 0xE0;
static const ubyte ENVELOPE_END = SEQ_END;
static const ubyte ENVELOPE_SUSTAIN = 0xE8;
static const ubyte ENVELOPE_SLIDE = 0xEA;

struct _FC_admin
{
    uword dmaFlags;  // which audio channels to turn on (AMIGA related)
    ubyte count;     // speed count
    ubyte speed;     // speed
    ubyte RScount;
    bool isEnabled;  // player on => true, else false
    
    struct _moduleOffsets
    {
        udword trackTable;
        udword patterns;
        udword sndModSeqs;
        udword volModSeqs;
        udword FC_silent;
    } offsets;

    int usedPatterns;
    int usedSndModSeqs;
    int usedVolModSeqs;
} 
FC_admin;

struct FC_SOUNDinfo_internal
{
    const ubyte* start;
    uword len, repOffs, repLen;
    // rest was place-holder (6 bytes)
};

struct _FC_SOUNDinfo
{
    // 10 samples/sample-packs
    // 80 waveforms
    FC_SOUNDinfo_internal snd[10+80];
}
FC_SOUNDinfo;

struct _FC_CHdata
{
    channel* ch;  // paula and mixer interface
    
    uword dmaMask;
    
    udword trackStart;     // track/step pattern table
    udword trackEnd;
    uword trackPos;

    udword pattStart;
    uword pattPos;
    
    sbyte transpose;       // TR
    sbyte soundTranspose;  // ST
    sbyte seqTranspose;    // from sndModSeq
    
    ubyte noteValue;
    
    sbyte pitchBendSpeed;
    ubyte pitchBendTime, pitchBendDelayFlag;
    
    ubyte portaInfo, portDelayFlag;
    sword portaOffs;
    
    udword volSeq;
    uword volSeqPos;
    
    ubyte volSlideSpeed, volSlideTime, volSustainTime;
    
    ubyte envelopeSpeed, envelopeCount;
    
    udword sndSeq;
    uword sndSeqPos;
    
    ubyte sndModSustainTime;
    
    ubyte vibFlag, vibDelay, vibSpeed,
         vibAmpl, vibCurOffs, volSlideDelayFlag;
    
    sbyte volume;
    uword period;
    
    const ubyte* pSampleStart;
    uword repeatOffset;
    uword repeatLength;
    uword repeatDelay;
};

#endif  // FC_DEFS_H
