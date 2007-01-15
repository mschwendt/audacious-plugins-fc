#ifndef MYTYPES_H
#define MYTYPES_H


#include "Config.h"

/* A ``bool'' type for compilers that don't (yet) support one. */
#ifndef FC_HAVE_BOOL
  typedef int bool;

  #if defined(true) || defined(false)
    #error Better check include file ``mytypes.h''.
    #undef true
    #undef false
  #endif
  #define true 1
  #define false 0
#endif  /* FC_HAVE_BOOL */


/* Wanted: 8-bit signed/unsigned. */
#if SIZEOF_CHAR > 1
	#error Platform unsupported.
#endif  /* SIZEOF_CHAR */
typedef signed char sbyte;
typedef unsigned char ubyte;

// Wanted: 16-bit signed/unsigned.
#if SIZEOF_SHORT_INT >= 2
typedef signed short int sword;
typedef unsigned short int uword;
#else
typedef signed int sword;
typedef unsigned int uword;
#endif  /* SIZEOF_SHORT_INT */

// Wanted: 32-bit signed/unsigned.
#if SIZEOF_INT >= 4
typedef signed int sdword;
typedef unsigned int udword;
#elif SIZEOF_LONG_INT >= 4
typedef signed long int sdword;
typedef unsigned long int udword;
#else
#error Platform not supported.
#endif  /* SIZEOF_INT */

// Some common type shortcuts.
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int ulong;


typedef void (*ptr2func)();


#endif
