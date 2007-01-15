// --------------------------------------------------------------------------
// AMIGA Future Composer v1.0 - v1.4 music player
//
// Implemented in 1997 by Michael Schwendt (mschwendt@users.sf.net).
//
// Based mainly on hand-written notes from 1990 which were made upon fixing
// a variety of bugs in the original player source code which was released
// with Future Composer.
//
//  http://xmms-fc.sf.net/
//
// No effort has been put into making this implementation elegant or
// efficient in either way.
// --------------------------------------------------------------------------

#if defined(DEBUG) || defined(DEBUG2) || defined(DEBUG3)
#include <iomanip.h>
#include <iostream.h>
#include "Dump.h"
#endif

#include "FC.h"
#include "FC_Defs.h"

bool FC_songEnd;     // whether song end has been reached

static smartPtr<ubyte> fcBuf;   // for safe unsigned access
static smartPtr<sbyte> fcBufS;  // for safe signed access

static bool isSMOD;  // whether file is in Future Composer 1.0 - 1.3 format
static bool isFC14;  // whether file is in Future Composer 1.4 format

static _FC_CHdata FC_CHdata[4];

// This array will be moved behind the input file. So don't forget
// to allocate additional sizeof(..) bytes.
static const ubyte FC_silent_data[8] =
{
    // Used as ``silent'' volume and modulation sequence.
    // seqSpeed, sndSeq, vibSpeed, vibAmp, vibDelay, initial Volume, ...
    //
    // Silent volume sequence starts here.
    0x01,
    // Silent modulation sequence starts here.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, SEQ_END
};

// Index is AND 0x7f.
static const uword FC_periods[(5+6)*12+4] = 
{
    0x06b0, 0x0650, 0x05f4, 0x05a0, 0x054c, 0x0500, 0x04b8, 0x0474,
    0x0434, 0x03f8, 0x03c0, 0x038a, 
    // +0x0c (*2 = byte-offset)
    0x0358, 0x0328, 0x02fa, 0x02d0, 0x02a6, 0x0280, 0x025c, 0x023a, 
    0x021a, 0x01fc, 0x01e0, 0x01c5,
    // +0x18 (*2 = byte-offset)
    0x01ac, 0x0194, 0x017d, 0x0168, 0x0153, 0x0140, 0x012e, 0x011d,
    0x010d, 0x00fe, 0x00f0, 0x00e2, 
    // +0x24 (*2 = byte-offset)
    0x00d6, 0x00ca, 0x00be, 0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f,
    0x0087, 0x007f, 0x0078, 0x0071,
    // +0x30 (*2 = byte-offset)
    0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071,
    0x0071, 0x0071, 0x0071, 0x0071, 
    // +0x3c (*2 = byte-offset)
    0x0d60, 0x0ca0, 0x0be8, 0x0b40, 0x0a98, 0x0a00, 0x0970, 0x08e8,
    0x0868, 0x07f0, 0x0780, 0x0714,
    // +0x48 (*2 = byte-offset)
    //
    // (NOTE) 0x49 = PATTERN BREAK, so this extra low octave is quite
    // useless for direct access. Transpose values would be required.
    // However, the FC player has hardcoded 0x0d60 as the lowest
    // sample period and does a range-check prior to writing a period
    // to the AMIGA custom chip. In short: This octave is useless!
    //
    0x1ac0, 0x1940, 0x17d0, 0x1680, 0x1530, 0x1400, 0x12e0, 0x11d0,
    0x10d0, 0x0fe0, 0x0f00, 0x0e28,
    //    
    // +0x54 (*2 = byte-offset)
    //
    // End of Future Composer 1.0 - 1.3 period table.
    //
    0x06b0, 0x0650, 0x05f4, 0x05a0, 0x054c, 0x0500, 0x04b8, 0x0474,
    0x0434, 0x03f8, 0x03c0, 0x038a,
    // +0x60 (*2 = byte-offset)
    0x0358, 0x0328, 0x02fa, 0x02d0, 0x02a6, 0x0280, 0x025c, 0x023a,
    0x021a, 0x01fc, 0x01e0, 0x01c5,
    // +0x6c (*2 = byte-offset)
    0x01ac, 0x0194, 0x017d, 0x0168,	0x0153, 0x0140, 0x012e, 0x011d,
    0x010d, 0x00fe, 0x00f0, 0x00e2,
    // +0x78 (*2 = byte-offset)
    0x00d6, 0x00ca, 0x00be, 0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f,
    // +0x80 (*2 = byte-offset), everything from here on is unreachable.
    0x0087, 0x007f, 0x0078, 0x0071
    // +0x84 (*2 = byte-offset)
};

static const uword SMOD_waveInfo[47*4] =
{
    0x0000, 0x0010, 0x0000, 0x0010,
    0x0020, 0x0010, 0x0000, 0x0010,
    0x0040, 0x0010, 0x0000, 0x0010,
    0x0060, 0x0010, 0x0000, 0x0010,
    0x0080, 0x0010, 0x0000, 0x0010,
    0x00a0, 0x0010, 0x0000, 0x0010,
    0x00c0, 0x0010, 0x0000, 0x0010,
    0x00e0, 0x0010, 0x0000, 0x0010,
    0x0100, 0x0010, 0x0000, 0x0010,
    0x0120, 0x0010, 0x0000, 0x0010,
    0x0140, 0x0010, 0x0000, 0x0010,
    0x0160, 0x0010, 0x0000, 0x0010,
    0x0180, 0x0010, 0x0000, 0x0010,
    0x01a0, 0x0010, 0x0000, 0x0010,
    0x01c0, 0x0010, 0x0000, 0x0010,
    0x01e0, 0x0010, 0x0000, 0x0010,
    0x0200, 0x0010, 0x0000, 0x0010,
    0x0220, 0x0010, 0x0000, 0x0010,
    0x0240, 0x0010, 0x0000, 0x0010,
    0x0260, 0x0010, 0x0000, 0x0010,
    0x0280, 0x0010, 0x0000, 0x0010,
    0x02a0, 0x0010, 0x0000, 0x0010,
    0x02c0, 0x0010, 0x0000, 0x0010,
    0x02e0, 0x0010, 0x0000, 0x0010,
    0x0300, 0x0010, 0x0000, 0x0010,
    0x0320, 0x0010, 0x0000, 0x0010,
    0x0340, 0x0010, 0x0000, 0x0010,
    0x0360, 0x0010, 0x0000, 0x0010,
    0x0380, 0x0010, 0x0000, 0x0010,
    0x03a0, 0x0010, 0x0000, 0x0010,
    0x03c0, 0x0010, 0x0000, 0x0010,
    0x03e0, 0x0010, 0x0000, 0x0010,
    0x0400, 0x0008, 0x0000, 0x0008,
    0x0410, 0x0008, 0x0000, 0x0008,
    0x0420, 0x0008, 0x0000, 0x0008,
    0x0430, 0x0008, 0x0000, 0x0008,
    0x0440, 0x0008, 0x0000, 0x0008,
    0x0450, 0x0008, 0x0000, 0x0008,
    0x0460, 0x0008, 0x0000, 0x0008,
    0x0470, 0x0008, 0x0000, 0x0008,
    0x0480, 0x0010, 0x0000, 0x0010,
    0x04a0, 0x0008, 0x0000, 0x0008,
    0x04b0, 0x0010, 0x0000, 0x0010,
    0x04d0, 0x0010, 0x0000, 0x0010,
    0x04f0, 0x0008, 0x0000, 0x0008,
    0x0500, 0x0008, 0x0000, 0x0008,
    0x0510, 0x0018, 0x0000, 0x0018
};

static const ubyte SMOD_waveforms[] =
{
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0x3f,0x37,0x2f,0x27,0x1f,0x17,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0x37,0x2f,0x27,0x1f,0x17,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0x2f,0x27,0x1f,0x17,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0x27,0x1f,0x17,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0x1f,0x17,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x17,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x0f,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x07,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0xff,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x88,0x0f,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x88,0x90,0x17,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x88,0x90,0x98,0x1f,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x88,0x90,0x98,0xa0,0x27,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x88,0x90,0x98,0xa0,0xa8,0x2f,0x37,
    0xc0,0xc0,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,0x00,0xf8,0xf0,0xe8,0xe0,0xd8,0xd0,0xc8,
    0xc0,0xb8,0xb0,0xa8,0xa0,0x98,0x90,0x88,0x80,0x88,0x90,0x98,0xa0,0xa8,0xb0,0x37,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,0x7f,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
    0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x7f,0x7f,0x7f,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x7f,0x7f,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x7f,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x80,0x80,0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x80,0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x80,0x80,0x90,0x98,0xa0,0xa8,0xb0,0xb8,0xc0,0xc8,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,
    0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x70,0x7f,
    0x80,0x80,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,
    0x45,0x45,0x79,0x7d,0x7a,0x77,0x70,0x66,0x61,0x58,0x53,0x4d,0x2c,0x20,0x18,0x12,
    0x04,0xdb,0xd3,0xcd,0xc6,0xbc,0xb5,0xae,0xa8,0xa3,0x9d,0x99,0x93,0x8e,0x8b,0x8a,
    0x45,0x45,0x79,0x7d,0x7a,0x77,0x70,0x66,0x5b,0x4b,0x43,0x37,0x2c,0x20,0x18,0x12,
    0x04,0xf8,0xe8,0xdb,0xcf,0xc6,0xbe,0xb0,0xa8,0xa4,0x9e,0x9a,0x95,0x94,0x8d,0x83,
    0x00,0x00,0x40,0x60,0x7f,0x60,0x40,0x20,0x00,0xe0,0xc0,0xa0,0x80,0xa0,0xc0,0xe0,
    0x00,0x00,0x40,0x60,0x7f,0x60,0x40,0x20,0x00,0xe0,0xc0,0xa0,0x80,0xa0,0xc0,0xe0,
    0x80,0x80,0x90,0x98,0xa0,0xa8,0xb0,0xb8,0xc0,0xc8,0xd0,0xd8,0xe0,0xe8,0xf0,0xf8,
    0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x70,0x7f,
    0x80,0x80,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70
};

// --------------------------------------------------------------------------

void FC_off()
{
    // (AMIGA) Power-LED on = low-pass filter on.
    
    FC_admin.isEnabled = false;
    
    for (int c=0; c<4; c++)
    {
        FC_CHdata[c].ch->off();
        FC_CHdata[c].ch->paula.period = 0;
        FC_CHdata[c].ch->paula.volume = 0;
        FC_CHdata[c].ch->updatePerVol();
    }
}

bool FC_init(void* data, udword dataLen, int startStep, int endStep)
{
    // Set up smart pointers for signed and unsigned input buffer access.
    // Ought to be read-only (const), but this implementation appends
    // a few values to the end of the buffer (see further below).
    fcBufS.setBuffer((sbyte*)data,dataLen);
    fcBuf.setBuffer((ubyte*)data,dataLen);
    
    // Check for "SMOD" ID (Future Composer 1.0 to 1.3).
    isSMOD = (fcBuf[0]==0x53 && fcBuf[1]==0x4D &&
              fcBuf[2]==0x4F && fcBuf[3]==0x44 &&
              fcBuf[4]==0x00);
    
    // Check for "FC14" ID (Future Composer 1.4).
    isFC14 = (fcBuf[0]==0x46 && fcBuf[1]==0x43 &&
              fcBuf[2]==0x31 && fcBuf[3]==0x34 &&
              fcBuf[4]==0x00);

    // (NOTE) A very few hacked "SMOD" modules exist which contain an ID
    // string "FC13". Although this could be supported easily, it should
    // NOT. Format detection must be able to rely on the ID field. As no
    // version of Future Composer has ever created a "FC13" ID, such hacked
    // modules are likely to be incompatible in other parts due to incorrect
    // conversion, e.g. effect parameters. It is like creating non-standard
    // module files whose only purpose is to confuse accurate music players.
    
    if (!isSMOD && !isFC14)
    {
        return false;
    }
    
    // (NOTE) This next bit is just for convenience.
    //
    // It is the only place where the module buffer is written to.
    //
    // Copy ``silent'' modulation sequence to end of FC module buffer so it
    // is in the address space of the FC module and thus allows using the
    // same smart pointers as throughout the rest of the code. Assume file
    // loader has provided that extra space at the end.
    FC_admin.offsets.FC_silent = dataLen-sizeof(FC_silent_data);
    for (unsigned int i=0; i<sizeof(FC_silent_data); i++)
    {
        fcBuf[FC_admin.offsets.FC_silent+i] = FC_silent_data[i];
    }
    
    // (AMIGA) Power-LED off => low-pass filter off.

    FC_admin.isEnabled = false;
    FC_admin.dmaFlags = 0;
    
    // This one later on gets incremented prior to first comparison
    // (4+1=5 => get speed at first step).
    FC_admin.RScount = 4;
    // (NOTE) Some FC implementations instead count from 0 to 4.
    
    // (NOTE) Instead of addresses (pointers) use 32-bit offsets everywhere.
    // This is just for added safety and convenience. One could range-check
    // each pointer/offset where appropriate to avoid segmentation faults
    // caused by damaged input data.
    
    if (isSMOD)
    {
        FC_admin.offsets.trackTable = SMOD_SONGTAB_OFFSET;
    }
    else  // if (isFC14)
    {
        FC_admin.offsets.trackTable = FC14_SONGTAB_OFFSET;
    }

#if defined(DEBUG)
    cout << "Track table (sequencer): " << endl;
    dumpLines(fcBuf,FC_admin.offsets.trackTable,
              readEndian(fcBuf[4],fcBuf[5],fcBuf[6],fcBuf[7]),
              TRACKTAB_ENTRY_LENGTH);
    cout << endl;
#endif

    // At +8 is offset to pattern data.
    FC_admin.offsets.patterns = readEndian(fcBuf[8],fcBuf[9],
                                           fcBuf[10],fcBuf[11]);
    // At +12 is length of patterns.
    // Divide by pattern length to get number of used patterns.
    // The editor is limited to 128 patterns.
    FC_admin.usedPatterns = readEndian(fcBuf[12],fcBuf[13],
                                      fcBuf[14],fcBuf[15])/PATTERN_LENGTH;
#if defined(DEBUG)
    cout << "Patterns: " << hex << FC_admin.usedPatterns << endl;
    dumpBlocks(fcBuf,FC_admin.offsets.patterns,
               FC_admin.usedPatterns*PATTERN_LENGTH,PATTERN_LENGTH);
#endif

    // At +16 is offset to first sound modulation sequence.
    FC_admin.offsets.sndModSeqs = readEndian(fcBuf[16],fcBuf[17],
                                            fcBuf[18],fcBuf[19]);
    // At +20 is total length of sound modulation sequences.
    // Divide by sequence length to get number of used sequences.
    // Each sequence is 64 bytes long.
    FC_admin.usedSndModSeqs = readEndian(fcBuf[20],fcBuf[21],
                                        fcBuf[22],fcBuf[23])/64;
#if defined(DEBUG)
    cout << "Sound modulation sequences: "
        << hex << FC_admin.usedSndModSeqs << endl;
    dumpBlocks(fcBuf,FC_admin.offsets.sndModSeqs,
               FC_admin.usedSndModSeqs*64,64);
#endif
    
    // At +24 is offset to first volume modulation sequence.
    FC_admin.offsets.volModSeqs = readEndian(fcBuf[24],fcBuf[25],
                                            fcBuf[26],fcBuf[27]);
    // At +28 is total length of volume modulation sequences.
    // Divide by sequence length to get number of used sequences.
    // Each sequence is 64 bytes long.
    FC_admin.usedVolModSeqs = readEndian(fcBuf[28],fcBuf[29],
                                        fcBuf[30],fcBuf[31])/64;
#if defined(DEBUG)
    cout << "Volume modulation sequences: "
        << hex << FC_admin.usedVolModSeqs << endl;
    dumpBlocks(fcBuf,FC_admin.offsets.volModSeqs,
               FC_admin.usedVolModSeqs*64,64);
#endif

#if defined(DEBUG)
    cout << "Samples:" << endl;
#endif
    // At +32 is module offset to start of samples.
    udword sampleOffset = readEndian(fcBuf[32],fcBuf[33],
                                     fcBuf[34],fcBuf[35]);
    udword sampleHeader = FC14_SMPHEADERS_OFFSET;
    // Max. 10 samples ($0-$9) or 10 sample-packs of 10 samples each.
    // Maximum sample length = 50000.
    // Previously: 32KB.
    // Total: 100000 (gee, old times).
    //
    // Sample length in words, repeat offset in bytes (but even),
    // Repeat length (in words, min=1). But in editor: all in bytes!
    //
    // One-shot sample (*recommended* method):
    // repeat offset = length*2 (to zero word at end of sample) and
    // replength = 1
    for (int sam = 0; sam < 10; sam++)
    {
        FC_SOUNDinfo.snd[sam].start = sampleOffset+fcBuf.tellBegin();
        // Sample length in words.
        uword sampleLength = readEndian(fcBuf[sampleHeader],
                                        fcBuf[sampleHeader+1]);
        FC_SOUNDinfo.snd[sam].len =  sampleLength;
        FC_SOUNDinfo.snd[sam].repOffs = readEndian(fcBuf[sampleHeader+2],
                                                   fcBuf[sampleHeader+3]);
        FC_SOUNDinfo.snd[sam].repLen = readEndian(fcBuf[sampleHeader+4],
                                                  fcBuf[sampleHeader+5]);

        // Safety treatment of "one-shot" samples.
        // 
        // We erase a word (two sample bytes) in the right place to
        // ensure that one-shot samples do not beep at their end
        // when looping on that part of the sample.
        //
        // It might not be strictly necessary to do this as it is
        // documented that FC is supposed to do this. But better be
        // safe than sorry.
        //
        // It is done because a cheap mixer is implemented which can
        // be used in the same way AMIGA custom chip Paula is used to
        // play quick-and-dirty one-shot samples.
        //
        // (NOTE) There is a difference in how one-shot samples are treated
        // in Future Composer 1.4 in comparison with older versions.
        
        if (isSMOD)
        {
            // Check whether this is a one-shot sample.
            if (FC_SOUNDinfo.snd[sam].repLen==1)
            {
                fcBuf[sampleOffset] = fcBuf[sampleOffset+1] = 0;
            }
        }

        // Skip to start of next sample data.
        sampleOffset += sampleLength;
        sampleOffset += sampleLength;
        
        if (isFC14)
        {
            udword pos = sampleOffset;
            // Make sure we do not erase the sample-pack ID "SSMP"
            // and check whether this is a one-shot sample.
            if (fcBuf[pos]!=0x53 && fcBuf[pos+1]!=0x53 &&
                fcBuf[pos+2]!=0x4D && fcBuf[pos+3]!=0x50 &&
                FC_SOUNDinfo.snd[sam].repLen==1)
            {
                fcBuf[pos] = fcBuf[pos+1] = 0;
            }
            // FC 1.4 keeps silent word behind sample.
            // Now skip that one to the next sample.
            //
            // (BUG-FIX) Add +2 to sample address is incorrect
            // for unused (i.e. empty) samples.
            //
            if (sampleLength != 0)
                sampleOffset += 2;
        }
        
        sampleHeader += 6;  // skip unused rest of header
        
#if defined(DEBUG)
        cout
            << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].start-(udword)fcBuf.tellBegin() << " "
            << dec << setw(4) << (int)FC_SOUNDinfo.snd[sam].len*2L << " "
            << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].repOffs << " "
            << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].repLen*2L << " "
            << endl;
        dumpLines(fcBuf,(udword)FC_SOUNDinfo.snd[sam].start-(udword)fcBuf.tellBegin(),
                  16,16);
#endif
    }

#if defined(DEBUG)
    cout << "Waveforms:" << endl;
#endif
    // 80 waveforms ($0a-$59), max $100 bytes length each.
    if (isSMOD)
    {
        // Old FC has built-in waveforms.
        const ubyte* wavePtr = SMOD_waveforms;
        int infoIndex = 0;
        for (int wave = 0; wave < 47; wave++)
        {
            int sam = 10+wave;
            FC_SOUNDinfo.snd[sam].start = wavePtr+SMOD_waveInfo[infoIndex++];
            FC_SOUNDinfo.snd[sam].len = SMOD_waveInfo[infoIndex++];
            FC_SOUNDinfo.snd[sam].repOffs = SMOD_waveInfo[infoIndex++];
            FC_SOUNDinfo.snd[sam].repLen = SMOD_waveInfo[infoIndex++];
#if defined(DEBUG)
            cout
                << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].start-(udword)wavePtr << " "
                << dec << setw(4) << (int)FC_SOUNDinfo.snd[sam].len*2L << " "
                << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].repOffs << " "
                << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].repLen*2L << " "
                << endl;
#endif
        }
    }
    else  //if (isFC14)
    {
        // At +36 is module offset to start of waveforms.
        udword waveOffset = readEndian(fcBuf[36],fcBuf[37],
                                       fcBuf[38],fcBuf[39]);
        // Module offset to array of waveform lengths.
        udword waveHeader = FC14_WAVEHEADERS_OFFSET;
        for (int wave = 0; wave < 80; wave++)
        {
            int sam = 10+wave;
            FC_SOUNDinfo.snd[sam].start = waveOffset+fcBuf.tellBegin();
            ubyte waveLength = fcBuf[waveHeader++];
            waveOffset += waveLength;
            waveOffset += waveLength;
            FC_SOUNDinfo.snd[sam].len = waveLength;
            FC_SOUNDinfo.snd[sam].repOffs = 0;
            FC_SOUNDinfo.snd[sam].repLen = waveLength;
#if defined(DEBUG)
            cout
                << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].start-(udword)fcBuf.tellBegin() << " "
                << dec << setw(4) << (int)FC_SOUNDinfo.snd[sam].len*2L << " "
                << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].repOffs << " "
                << dec << setw(6) << (int)FC_SOUNDinfo.snd[sam].repLen*2L << " "
                << endl;
#endif
        }
    }

    // At +4 is length of track table.
    udword trackTabLen = readEndian(fcBuf[4],fcBuf[5],fcBuf[6],fcBuf[7]);
#if defined(DEBUG)
    cout << "trackTabLen = " << hex << setw(8) << setfill('0') << trackTabLen << endl;
#endif
    for (int chan = 0; chan < 4; chan++)
    {
        FC_CHdata[chan].dmaMask = (1<<chan);
        FC_CHdata[chan].trackPos =
            FC_CHdata[chan].pattPos = 0;
        FC_CHdata[chan].volSlideSpeed =
            FC_CHdata[chan].volSlideTime =
            FC_CHdata[chan].volSlideDelayFlag =
            FC_CHdata[chan].volSustainTime = 0;
        FC_CHdata[chan].envelopeCount =
            FC_CHdata[chan].envelopeSpeed = 1;
        FC_CHdata[chan].vibSpeed =
            FC_CHdata[chan].vibDelay =
            FC_CHdata[chan].vibAmpl =
            FC_CHdata[chan].vibCurOffs =
            FC_CHdata[chan].vibFlag = 0;
        FC_CHdata[chan].pitchBendSpeed = 
            FC_CHdata[chan].pitchBendTime =
            FC_CHdata[chan].pitchBendDelayFlag = 0;
        FC_CHdata[chan].transpose =
            FC_CHdata[chan].seqTranspose = 0;
        FC_CHdata[chan].portaInfo =
            FC_CHdata[chan].portaOffs =
            FC_CHdata[chan].portDelayFlag = 0;
        FC_CHdata[chan].volSeq = FC_admin.offsets.FC_silent;
        FC_CHdata[chan].sndSeq = FC_admin.offsets.FC_silent+1;
        FC_CHdata[chan].volSeqPos =
            FC_CHdata[chan].sndSeqPos = 0;
        FC_CHdata[chan].sndModSustainTime = 0;
        FC_CHdata[chan].noteValue = 0;
        FC_CHdata[chan].volume = 0;

        // The interface to a cheap Paula simulator/mixer.
        extern channel logChannel[32];
        FC_CHdata[chan].ch = &logChannel[chan];
        FC_CHdata[chan].ch->off();
        FC_CHdata[chan].ch->paula.start = fcBuf.tellBegin();  // 0
        // (NOTE) Some implementations set this to 0x0100.
        FC_CHdata[chan].ch->paula.length = 1;
        FC_CHdata[chan].ch->takeNextBuf();
        FC_CHdata[chan].ch->paula.period = 0;
        FC_CHdata[chan].ch->paula.volume = 0;
        FC_CHdata[chan].ch->updatePerVol();

        // Track table start and end.
        FC_CHdata[chan].trackStart = FC_admin.offsets.trackTable+chan*3;
        FC_CHdata[chan].trackEnd = FC_CHdata[chan].trackStart+trackTabLen;
        FC_CHdata[chan].trackPos = TRACKTAB_ENTRY_LENGTH*startStep;

        // 4*
        // PT = PATTERN
        // TR = TRANSPOSE
        // ST = SOUND TRANSPOSE
        //
        // 1*
        // RS = REPLAY SPEED
        
        // Read PT/TR/ST from track table.
        udword trackTabPos = FC_CHdata[chan].trackStart+FC_CHdata[chan].trackPos;
        uword pattern = fcBuf[trackTabPos++];  // PT
        FC_CHdata[chan].pattStart = FC_admin.offsets.patterns+(pattern<<6);
        FC_CHdata[chan].transpose = fcBuf[trackTabPos++];  // TR
        FC_CHdata[chan].soundTranspose = fcBufS[trackTabPos];   // ST
    }

    // Get and set song speed.
    //
    // (BUG-FIX) Some FC players here read the speed from the first step.
    // This is the wrong speed if a sub-song is selected by skipping steps.
    // 
    // (NOTE) If it is skipped to a step where no replay step is specified,
    // the default speed is taken. This can be wrong. The only solution
    // would be to fast-forward the song, i.e. read the speed from all
    // steps up to the starting step.
    //
    ubyte songSpeed = fcBuf[FC_CHdata[0].trackStart+FC_CHdata[0].trackPos+12];
    if (songSpeed == 0)
        songSpeed = 3;  // documented default
    
    FC_admin.count = FC_admin.speed = songSpeed;
    FC_admin.isEnabled = true;
    FC_songEnd = false;
    
    //
    
    mixerPlayRout = &FC_play;  // this would be called upon interrupt
    
    MIXER_voices = 4;
    MIXER_samples = 128;
    MIXER_speed = FC_admin.speed;  // set default playing speed

    // (NOTE) The lowest octave in the period table is unreachable
    // due to a hardcoded range-check (see bottom).
    
    if (isSMOD)
    {
        MIXER_minPeriod = 0x0071;
        MIXER_maxPeriod = 0x0d60;
    }
    else  //if (isFC14)
    {
        MIXER_minPeriod = 0x0071;
        MIXER_maxPeriod = 0x0d60;
    }
    
    for (int c=0; c<4; c++)
    {
        FC_CHdata[c].ch->sampleFrequency = 11200;
        FC_CHdata[c].ch->bitsPerSample = 8;
        FC_CHdata[c].ch->sign = true;
        FC_CHdata[c].ch->looping = true;
    }

    if (isSMOD)
        mixerFormatName = "Future Composer 1.0 - 1.3 (AMIGA)";
    else if (isFC14)
        mixerFormatName = "Future Composer 1.4 (AMIGA)";
    else
        mixerFormatName = "unknown format";

    return isFC14||isSMOD;
}

// --------------------------------------------------------------------------
 
void FC_play()
{
    if (!FC_admin.isEnabled)  // on/off flag
        return;
    
    if (--FC_admin.count == 0)
    {
        FC_admin.count = FC_admin.speed;  // reload

        void FC_nextNote(_FC_CHdata&);
        // Prepare next note for each voice.
        FC_nextNote(FC_CHdata[0]);
        FC_nextNote(FC_CHdata[1]);
        FC_nextNote(FC_CHdata[2]);
        FC_nextNote(FC_CHdata[3]);
#if defined(DEBUG2)
        cout << endl << flush;
#endif
    }
    
    // Procedure calls in next loop will decide which
    // audio channel to turn on.
    FC_admin.dmaFlags = 0;

    for (int c = 0; c < 4; c++)
    {
        void FC_processModulation(_FC_CHdata&);
        // Start or update instrument.
        FC_processModulation(FC_CHdata[c]);
        
        FC_CHdata[c].ch->paula.period = FC_CHdata[c].period;
        FC_CHdata[c].ch->paula.volume = FC_CHdata[c].volume;
        FC_CHdata[c].ch->updatePerVol();
        
        if (FC_CHdata[c].repeatDelay != 0)
        {
            if (--FC_CHdata[c].repeatDelay == 1)
            {
                FC_CHdata[c].repeatDelay = 0;
                FC_CHdata[c].ch->paula.start = FC_CHdata[c].pSampleStart +
                    FC_CHdata[c].repeatOffset;
                FC_CHdata[c].ch->paula.length = FC_CHdata[c].repeatLength;
                FC_CHdata[c].ch->takeNextBuf();
            }
        }
    }
 
    // Finally decide which audio channels to start.
    // This could be moved into previous loop.
    for (int c = 0; c < 4; c++)
    {
        // Enable channel? Else, do not touch it.
        if ((FC_admin.dmaFlags&(1<<c)) != 0)
        {
            FC_CHdata[c].ch->on();
        }
    }
}

// --------------------------------------------------------------------------

void FC_nextNote(_FC_CHdata& FC_CHXdata)
{
    // Get offset to (or address of) current pattern position.
    udword pattOffs = FC_CHXdata.pattStart+FC_CHXdata.pattPos;

    // Check for pattern end or whether pattern BREAK
    // command is set.
    if (FC_CHXdata.pattPos==PATTERN_LENGTH
        || (isFC14 && fcBuf[pattOffs]==PATTERN_BREAK))
    {
        // End pattern.
        
#if defined(DEBUG3)
        if (fcBuf[pattOffs] == PATTERN_BREAK)
            cout << "--- PATTERN BREAK ---" << endl;
#endif
        
        // (NOTE) In order for pattern break to work correctly, the
        // pattern break value 0x49 must be at the same position in
        // each of the four patterns which are currently activated
        // for the four voices.
        //
        // Alternatively, one could advance all voices to the next
        // track step here in a 4-voice loop to make sure voices
        // stay in sync.
        
        FC_CHXdata.pattPos = 0;
        
        // Advance one step in track table.
        FC_CHXdata.trackPos += TRACKTAB_ENTRY_LENGTH;  // 0x000d
        udword trackOffs = FC_CHXdata.trackStart+FC_CHXdata.trackPos;
        
        // (BUG-FIX) Some FC players here apply a normal
        // compare-if-equal which is not accurate enough and
        // can cause the player to step beyond the song end.
        //
        // (BUG-FIX) Some FC14 modules have a pattern table length
        // which is not a multiple of 13. Hence we check whether
        // the currently activated table line would fit.
        
        if ((trackOffs+12) >= FC_CHXdata.trackEnd)  // pattern table end?
        {
            FC_CHXdata.trackPos = 0;     // restart by default
            trackOffs = FC_CHXdata.trackStart;

            FC_songEnd = true;
            
            // (NOTE) Some songs better stop here or reset all
            // channels to cut off any pending sounds.
        }

        // Step   Voice 1    Voice 2    Voice 3    Voice 4    Speed
        // SP     PT TR ST   PT TR ST   PT TR ST   PT TR ST   RS
        //
        // SP = STEP
        // PT = PATTERN
        // TR = TRANSPOSE
        // ST = SOUND TRANSPOSE
        // RS = REPLAY SPEED

        // Decide whether to read new song speed.
        if (++FC_admin.RScount == 5)
        {
            FC_admin.RScount = 1;
            ubyte newSpeed = fcBuf[trackOffs+12];  // RS (replay speed)
            if (newSpeed != 0)  // 0 would be underflow
            {
                FC_admin.count = FC_admin.speed = newSpeed;
            }
        }

        uword pattern = fcBuf[trackOffs++];  // PT
        FC_CHXdata.transpose = fcBufS[trackOffs++];
        FC_CHXdata.soundTranspose = fcBufS[trackOffs++];
        
        FC_CHXdata.pattStart = FC_admin.offsets.patterns+(pattern<<6);
        // Get new pattern pointer (pattPos is 0 already, see above).
        pattOffs = FC_CHXdata.pattStart;
    }

#if defined(DEBUG2)
    if (FC_CHXdata.dmaMask==1
        && FC_CHXdata.pattPos==0)
    {
        cout << endl;
        cout << "Step = " << hex << setw(4) << setfill('0') << FC_CHXdata.trackPos/ TRACKTAB_ENTRY_LENGTH;
        cout << " | " << hex << setw(5) << setfill('0') << (int)FC_CHXdata.trackStart << ", " << (int)(FC_CHXdata.trackStart+FC_CHXdata.trackPos) << ", " << (int)FC_CHXdata.trackEnd << endl;
        udword tmp = FC_CHXdata.trackStart+FC_CHXdata.trackPos;
        for (int t = 0; t < 13; ++t)
            cout << hex << setw(2) << setfill('0') << (int)fcBuf[tmp++] << ' ';
        cout << endl;
        cout << endl;
    }
    
    cout << hex << setw(2) << setfill('0') << (int)fcBuf[pattOffs] << ' ' 
        << setw(2) << (int)fcBuf[pattOffs+1];
    if (FC_CHXdata.dmaMask != 8)
        cout << " | ";
#endif

    // Process pattern entry.
    
    ubyte note = fcBuf[pattOffs++];
    ubyte info1 = fcBuf[pattOffs];  // info byte #1
    
    if (note != 0)
    {
        FC_CHXdata.portaOffs = 0;   // reset portamento offset
        FC_CHXdata.portaInfo = 0;   // stop port., erase old parameter
        
        // (BUG-FIX) Disallow signed underflow here.
        FC_CHXdata.noteValue = note&0x7f;
        
        // (NOTE) Since first note is 0x01, first period at array
        // offset 0 cannot be accessed directly (i.e. without adding
        // transpose values from track table or modulation sequence).
        
        // Disable channel right now.
        FC_CHXdata.ch->off();
        // Later enable channel.
        FC_admin.dmaFlags |= FC_CHXdata.dmaMask;

        // Pattern offset stills points to info byte #1.
        // Get instrument/volModSeq number from info byte #1
        // and add sound transpose value from track table.
        uword sound = (fcBuf[pattOffs]&0x3f)+FC_CHXdata.soundTranspose;
        //
        // (FC14 BUG-FIX) Better mask here to take care of overflow.
        //
        sound &= 0x3f;

        // (NOTE) Some FC players here put pattern info byte #1
        // into an unused byte variable at structure offset 9.
        
        udword seqOffs;  // the modulation sequence for this sound
        
        if (sound > (FC_admin.usedVolModSeqs-1))
        {
            seqOffs = FC_admin.offsets.FC_silent;
        }
        else
        {
            seqOffs = FC_admin.offsets.volModSeqs+(sound<<6);
        }
        FC_CHXdata.envelopeSpeed = FC_CHXdata.envelopeCount = fcBuf[seqOffs++];
        // Get sound modulation sequence number.
        sound = fcBuf[seqOffs++];
        FC_CHXdata.vibSpeed = fcBuf[seqOffs++];
        FC_CHXdata.vibFlag = 0x40;  // vibrato UP at start
        FC_CHXdata.vibAmpl = FC_CHXdata.vibCurOffs = fcBuf[seqOffs++];
        FC_CHXdata.vibDelay = fcBuf[seqOffs++];
        FC_CHXdata.volSeq = seqOffs;
        FC_CHXdata.volSeqPos = 0;
        FC_CHXdata.volSustainTime = 0;
        
        if (sound > (FC_admin.usedSndModSeqs-1))
        {
            // (NOTE) Silent sound modulation sequence is different
            // from silent instrument definition sequence.
            seqOffs = FC_admin.offsets.FC_silent+1;
        }
        else
        {
            seqOffs = FC_admin.offsets.sndModSeqs+(sound<<6);
        }
        FC_CHXdata.sndSeq = seqOffs;
        FC_CHXdata.sndSeqPos = 0;
        FC_CHXdata.sndModSustainTime = 0;
    }

    // Portamento: bit 7 set = ON, bit 6 set = OFF, bits 5-0 = speed
    // New note resets and clears portamento working values.

    if ((info1&0x40) != 0)          // portamento OFF?
    {
        FC_CHXdata.portaInfo = 0;   // stop port., erase old parameter
    }
    
    if ((info1&0x80) != 0)          // portamento ON?
    {
        //
        // (FC14 BUG-FIX) Kill portamento ON/OFF bits.
        //
        // Get portamento speed from info byte #2.
        // Info byte #2 is info byte #1 in next line of pattern,
        // Therefore the +2 offset.
        FC_CHXdata.portaInfo = fcBuf[pattOffs+2]&0x3f;
    }

    // Advance to next pattern entry.
    FC_CHXdata.pattPos += 2;
}

// --------------------------------------------------------------------------
// The order of func/proc calls might be confusing, but is necessary
// to simulate JMP instructions in the original player code without
// making use of ``goto''.

void FC_processModulation(_FC_CHdata&);
void FC_readModCommand(_FC_CHdata&);

void FC_processPerVol(_FC_CHdata&);

inline void FC_setWave(_FC_CHdata& FC_CHXdata, ubyte num)
{
    FC_CHXdata.pSampleStart = FC_SOUNDinfo.snd[num].start;
    FC_CHXdata.ch->paula.start = FC_SOUNDinfo.snd[num].start;
    FC_CHXdata.ch->paula.length = FC_SOUNDinfo.snd[num].len;
    FC_CHXdata.ch->takeNextBuf();
    FC_CHXdata.repeatOffset = FC_SOUNDinfo.snd[num].repOffs;
    FC_CHXdata.repeatLength = FC_SOUNDinfo.snd[num].repLen;
    FC_CHXdata.repeatDelay = 3;
}

inline void FC_readSeqTranspose(_FC_CHdata& FC_CHXdata)
{
    FC_CHXdata.seqTranspose = fcBufS[FC_CHXdata.sndSeq+FC_CHXdata.sndSeqPos];
    ++FC_CHXdata.sndSeqPos;
}

void FC_processModulation(_FC_CHdata& FC_CHXdata)
{
    if (FC_CHXdata.sndModSustainTime != 0)
    {
        --FC_CHXdata.sndModSustainTime;
        FC_processPerVol(FC_CHXdata);
        return;
    }
    FC_readModCommand(FC_CHXdata);
}

void FC_readModCommand(_FC_CHdata& FC_CHXdata)
{
    udword seqOffs = FC_CHXdata.sndSeq+FC_CHXdata.sndSeqPos;

    // (NOTE) After each command (except LOOP, END, SUSTAIN,
    // and NEWVIB) follows a transpose value.
    
    if (fcBuf[seqOffs] == SNDMOD_LOOP)
    {
        FC_CHXdata.sndSeqPos = fcBuf[seqOffs+1]&0x3f;
        // Calc new sequence address.
        seqOffs = FC_CHXdata.sndSeq+FC_CHXdata.sndSeqPos;
    }

    if (fcBuf[seqOffs] == SNDMOD_END)
    {
        FC_processPerVol(FC_CHXdata);
        return;
    }
    
    else if (fcBuf[seqOffs] == SNDMOD_SETWAVE)
    {
        // Disable channel right now.
        FC_CHXdata.ch->off();
        // Enable channel later.
        FC_admin.dmaFlags |= FC_CHXdata.dmaMask;
        // Restart envelope.
        FC_CHXdata.volSeqPos = 0;
        FC_CHXdata.envelopeCount = 1;
        
        FC_setWave(FC_CHXdata,fcBuf[seqOffs+1]);
        FC_CHXdata.sndSeqPos += 2;
        
        FC_readSeqTranspose(FC_CHXdata);
        
        FC_processPerVol(FC_CHXdata);
        return;
    }

    else if (fcBuf[seqOffs] == SNDMOD_CHANGEWAVE)
    {
        FC_setWave(FC_CHXdata,fcBuf[seqOffs+1]);
        FC_CHXdata.sndSeqPos += 2;
    
        FC_readSeqTranspose(FC_CHXdata);
        
        FC_processPerVol(FC_CHXdata);
        return;
    }

    else if (fcBuf[seqOffs] == SNDMOD_SETPACKWAVE)
    {
        // Disable channel right now.
        FC_CHXdata.ch->off();
        // Enable channel later.
        FC_admin.dmaFlags |= FC_CHXdata.dmaMask;
        
        uword i = fcBuf[seqOffs+1];  // sample/pack nr.
        if (i < 10)  // sample or waveform?
        {
            udword sndOffs = FC_SOUNDinfo.snd[i].start - fcBuf.tellBegin();
            // "SSMP"? sample-pack?
            if (fcBuf[sndOffs]==0x53 && fcBuf[sndOffs+1]==0x53 &&
                fcBuf[sndOffs+2]==0x4D && fcBuf[sndOffs+3]==0x50)
            {
                sndOffs += 4;
                // Skip header and 10*2 info blocks of size 16.
                udword smpStart = sndOffs+320;
                i = fcBuf[seqOffs+2];  // sample nr.
                i <<= 4;  // *16 (block size)
                sndOffs += i;
                smpStart += readEndian(fcBuf[sndOffs],fcBuf[sndOffs+1],
                                 fcBuf[sndOffs+2],fcBuf[sndOffs+3]);
                FC_CHXdata.pSampleStart = smpStart+fcBuf.tellBegin();
                FC_CHXdata.ch->paula.start = FC_CHXdata.pSampleStart;
                FC_CHXdata.ch->paula.length = readEndian(fcBuf[sndOffs+4],
                                                         fcBuf[sndOffs+5]);
                FC_CHXdata.ch->takeNextBuf();
                
                // (FC14 BUG-FIX): Players set period here by accident.
                // m68k code move.l 4(a2),4(a3), but 6(a3) is period.
                
                FC_CHXdata.repeatOffset = readEndian(fcBuf[sndOffs+6],
                                                     fcBuf[sndOffs+7]);
                FC_CHXdata.repeatLength = readEndian(fcBuf[sndOffs+8],
                                                     fcBuf[sndOffs+9]);
                if (FC_CHXdata.repeatLength == 1)
                {
                    // Erase first word behind sample to avoid beeping
                    // one-shot mode upon true emulation of Paula.
                    fcBuf[smpStart+FC_CHXdata.repeatOffset] = 0;
                    fcBuf[smpStart+FC_CHXdata.repeatOffset+1] = 0;
                }
                // Restart envelope.
                FC_CHXdata.volSeqPos = 0;
                FC_CHXdata.envelopeCount = 1;
                //
                FC_CHXdata.repeatDelay = 3;
            }
        }
        FC_CHXdata.sndSeqPos += 3;
        
        FC_readSeqTranspose(FC_CHXdata);
        
        FC_processPerVol(FC_CHXdata);
        return;
    }

    else if (fcBuf[seqOffs] == SNDMOD_NEWSEQ)
    {
        uword seq = fcBuf[seqOffs+1];
        FC_CHXdata.sndSeq = FC_admin.offsets.sndModSeqs+(seq<<6);
        FC_CHXdata.sndSeqPos = 0;
        // Recursive call (ought to be protected via a counter).
        FC_readModCommand(FC_CHXdata);
        return;
    }

    else if (fcBuf[seqOffs] == SNDMOD_SUSTAIN)
    {
        FC_CHXdata.sndModSustainTime = fcBuf[seqOffs+1];
        FC_CHXdata.sndSeqPos += 2;

        // Decrease sustain counter and decide whether to continue
        // to envelope modulation.
        // Recursive call (ought to be protected via a counter).
        FC_processModulation(FC_CHXdata);
        return;
    }

    else if (fcBuf[seqOffs] == SNDMOD_NEWVIB)
    {
        FC_CHXdata.vibSpeed = fcBuf[seqOffs+1];
        FC_CHXdata.vibAmpl = fcBuf[seqOffs+2];
        FC_CHXdata.sndSeqPos += 3;
        
        FC_processPerVol(FC_CHXdata);
        return;
    }

    else if (fcBuf[seqOffs] == SNDMOD_PITCHBEND)
    {
        FC_CHXdata.pitchBendSpeed = fcBufS[seqOffs+1];
        FC_CHXdata.pitchBendTime = fcBuf[seqOffs+2];
        FC_CHXdata.sndSeqPos += 3;
        
        FC_readSeqTranspose(FC_CHXdata);
    
        FC_processPerVol(FC_CHXdata);
        return;
    }

    else // Not a command, but a transpose value.
    {
        FC_readSeqTranspose(FC_CHXdata);
    
        FC_processPerVol(FC_CHXdata);
    }
}

// --------------------------------------------------------------------------
 
void FC_processPerVol(_FC_CHdata&);
void FC_volSlide(_FC_CHdata&);

// (NOTE) This part of the code is not protected against a deadlock
// caused by damaged music module data.

void FC_volSlide(_FC_CHdata& FC_CHXdata)
{
    // Following flag divides the volume sliding speed by two.
    FC_CHXdata.volSlideDelayFlag ^= 0xff;  // = NOT
    if (FC_CHXdata.volSlideDelayFlag != 0)
    {
        --FC_CHXdata.volSlideTime;
        FC_CHXdata.volume += FC_CHXdata.volSlideSpeed;
        if (FC_CHXdata.volume < 0)
        {
            FC_CHXdata.volume = FC_CHXdata.volSlideTime = 0;
        }
        
        // (NOTE) Most FC players do not check whether Paula's
        // maximum volume level is exceeded.
        
        if (FC_CHXdata.volume > 64)
        {
            FC_CHXdata.volume = 64;
            FC_CHXdata.volSlideTime = 0;
        }
    }
}

void FC_processPerVol(_FC_CHdata& FC_CHXdata)
{
    bool repeatVolSeq;  // JUMP/GOTO - WHILE conversion
    do
    {
        repeatVolSeq = false;
        
        // Sustain current volume level? NE => yes, EQ => no.
        if (FC_CHXdata.volSustainTime != 0)
        {
            --FC_CHXdata.volSustainTime;
        }

        // Slide volume? NE => yes, EQ => no.
        else if (FC_CHXdata.volSlideTime != 0)
        {
            FC_volSlide(FC_CHXdata);
        }

        // Time to set next volume level? NE => no, EQ => yes.
        else if (--FC_CHXdata.envelopeCount == 0)
        {
            FC_CHXdata.envelopeCount = FC_CHXdata.envelopeSpeed;

            bool readNextVal;  // JUMP/GOTO - WHILE conversion
            do
            {
                readNextVal = false;
                    
                udword seqOffs = FC_CHXdata.volSeq+FC_CHXdata.volSeqPos;
                ubyte command = fcBuf[seqOffs];

                switch (command)
                {
                 case ENVELOPE_SUSTAIN:
                    {
                        FC_CHXdata.volSustainTime = fcBuf[seqOffs+1];
                        FC_CHXdata.volSeqPos += 2;
                        // This shall loop to beginning of proc.
                        repeatVolSeq = true;
                        break;
                    }
                 case ENVELOPE_SLIDE:
                    {
                        FC_CHXdata.volSlideSpeed = fcBuf[seqOffs+1];
                        FC_CHXdata.volSlideTime = fcBuf[seqOffs+2];
                        FC_CHXdata.volSeqPos += 3;
                        FC_volSlide(FC_CHXdata);
                        break;
                    }
                 case ENVELOPE_LOOP:
                    {
                        // Range check should be done here.
                        FC_CHXdata.volSeqPos = (fcBuf[seqOffs+1]-5)&0x3f;
                        // (FC14 BUG) Some FC players here do not read a
                        // parameter at the new sequence position. They
                        // leave the pos value in d0, which then passes
                        // as parameter through all the command comparisons
                        // (this switch statement) in FC_effa() up to
                        // FC_effno().
                        readNextVal = true;
                        break;
                    }
                 case ENVELOPE_END:
                    {
                        break;
                    }
                 default:
                    {
                        // Read volume value and advance.
                        FC_CHXdata.volume = fcBuf[seqOffs];
                        ++FC_CHXdata.volSeqPos;
                        break;
                    }
                }
            }
            while (readNextVal);
        }
    }
    while (repeatVolSeq);
    
    // Now determine note and period value to play.
    
    sdword tmp0, tmp1;

    tmp0 = FC_CHXdata.seqTranspose;
    if (tmp0 >= 0)
    {
        tmp0 += FC_CHXdata.noteValue;
        tmp0 += FC_CHXdata.transpose;
        // (NOTE) Permit underflow at this point. Some modules
        // need it because--for some unknown reason--they work
        // with huge values such as transpose = 0x8c.
    }
    // else: lock note (i.e. transpose value from sequence is note to play)
    
#if defined(DEBUG2)
    if ((tmp0&0x7f)>0x53)
    {
        cout << "X ";
#if defined(DEBUG3)
        cout << "=== NOTE > 0x53 ===" << endl;
#endif
    }
#endif

    tmp0 &= 0x7f;
    tmp1 = tmp0<<1;           // *2 (later used to find octave)
    tmp0 = FC_periods[tmp0];

    // Vibrato.
    //
    // Vibrato offset changes between [0,1,...,2*vibAmpl]
    // Offset minus vibAmpl is value to apply.
    
    if (FC_CHXdata.vibDelay == 0)
    {
        uword noteTableOffset = tmp1;  // tmp1 is note*2;
    
        sword vibDelta = FC_CHXdata.vibAmpl;
        vibDelta <<= 1;  // pos/neg amplitude delta
    
        // vibFlag bit 5: 0 => vibrato down, 1 => vibrato up
        //
        // (NOTE) In the original player code the vibrato half speed delay
        // flag (D6) in bit 0 is toggled but never checked, because the
        // vibrato flag byte will never get negative.
    
        tmp1 = FC_CHXdata.vibCurOffs;

        if ((FC_CHXdata.vibFlag&(1<<5))==0)
        {
            tmp1 -= FC_CHXdata.vibSpeed;
            // Lowest value reached?
            if (tmp1 < 0)
            {
                tmp1 = 0;
                FC_CHXdata.vibFlag |= (1<<5);   // switch to vibrato up
            }
        }
        else
        {
            tmp1 += FC_CHXdata.vibSpeed;
            // Amplitude reached?
            if (tmp1 > vibDelta)
            {
                tmp1 = vibDelta;
                FC_CHXdata.vibFlag &= ~(1<<5);  // switch to vibrato down
            }
        }

        FC_CHXdata.vibCurOffs = tmp1;
    
        // noteTableOffset is note*2;
    
        tmp1 -= FC_CHXdata.vibAmpl;

        // Octave 5 at period table byte-offset 96 contains the highest
        // period only. 96+160 = 256. This next bit ensures that vibrato
        // does not exceed the five octaves in the period table.
        // Octave 6 (but lowest!) is FC14 only.
        noteTableOffset += 160;  // + $a0
        while (noteTableOffset < 256)
        {
            tmp1 <<= 1;    // double vibrato value for each octave
            noteTableOffset += 2*12;  // advance octave index
        };
        tmp0 += tmp1;  // apply vibrato to period
    
        // (NOTE) Questionable code here in the original player sources.
        // Although bit 0 of D6 is toggled, the code (see above) that
        // checks it is unreachable.
    }
    else
    {
        --FC_CHXdata.vibDelay;
    
        // (NOTE) Questionable code here in existing FC players. Although
        // bit 0 of D6 is toggled, the code that checks it is unreachable.
        // That bad code has not been converted.
    }

    // Portamento.

    // (NOTE) As of FC 1.4 portamento plays at half speed compared to
    // old versions.
    
    // Following flag divides the portamento speed by two
    // for FC14 modules.
    FC_CHXdata.portDelayFlag ^= 0xff;  // = NOT
    if (isSMOD || FC_CHXdata.portDelayFlag!=0)
    {
        sbyte param = FC_CHXdata.portaInfo;
        if (param != 0)
        {
            if (param > 0x1f)  // > 0x20 = portamento down
            {
                param &= 0x1f;
                param = (-param);
            }
            FC_CHXdata.portaOffs -= param;
        }
    }

    // Pitchbend.
    
    // Following flag divides the pitch bending speed by two.
    FC_CHXdata.pitchBendDelayFlag ^= 0xff;  // not
    if (FC_CHXdata.pitchBendDelayFlag != 0)
    {
        if (FC_CHXdata.pitchBendTime != 0)
        {
            --FC_CHXdata.pitchBendTime;
            sbyte speed = FC_CHXdata.pitchBendSpeed;
            if (speed != 0)
            {
                FC_CHXdata.portaOffs -= speed;
            }
        }
    }
    
    tmp0 += FC_CHXdata.portaOffs;
    if (tmp0 <= 0x0070)
    {
        tmp0 = 0x0071;
    }
    // (NOTE) This should be 0x1ac0, but the extra low octave has
    // been added in FC 1.4 and is a non-working hack due to this
    // range-check (see header file).
    if (tmp0 > 0x0d60)
    {
        tmp0 = 0x0d60;
    }
    FC_CHXdata.period = tmp0;
}
