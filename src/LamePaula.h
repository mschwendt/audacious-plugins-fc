#ifndef LAMEPAULA_H
#define LAMEPAULA_H

#include "MyTypes.h"

extern uword MIXER_patterns;
extern uword MIXER_samples;
extern uword MIXER_voices;
extern ubyte MIXER_speed;
extern ubyte MIXER_bpm;
extern ubyte MIXER_count;
extern uword MIXER_minPeriod;
extern uword MIXER_maxPeriod;

extern const char* mixerFormatName;
extern void (*mixerPlayRout)();

extern void mixerSetBpm( uword );

struct _paula
{
    // Paula
    const ubyte* start;  // start address
    uword length;        // length in 16-bit words
    uword period;
    uword volume;        // 0-64
};

class channel
{
 public:
    _paula paula;
    
    bool isOn;
    
    const ubyte* start;
    const ubyte* end;
    udword length;
    
    const ubyte* repeatStart;
    const ubyte* repeatEnd;
    udword repeatLength;
    
    ubyte bitsPerSample;
    uword volume;
    uword period;
    udword sampleFrequency;
    bool sign;
    bool looping;  // whether to loop sample buffer continously (PAULA emu)
    ubyte panning;
  //
    uword curPeriod;
    udword stepSpeed;
    udword stepSpeedPnt;
    udword stepSpeedAddPnt;

    void off()
    {
        isOn = false;
    }

    channel()
    {
        off();
    }
    
    void on();
    void takeNextBuf();    // take parameters from paula.* (or just to repeat.*)
    void updatePerVol();   // period, volume
};

#endif  // LAMEPAULA_H
