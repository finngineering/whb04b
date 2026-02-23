#ifndef DEFS_H
#define DEFS_H

#include "ch55x/ch554.h"


// Defines to avoid syntax highlighting issues in VS code.
// These are ignored at compilation because then __SDCC is defined
#ifndef __SDCC
    #define __code
    #define __xdata
    #define __data
    #define __pdata
    #define __at(x)
    #define __sbit
    #define __asm
    #define __endasm
    #define __interrupt(x)
    #define __using(x)
#endif

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

SBIT(DEBUG_PIN, 0xb0, 0); // P3.0

#endif
