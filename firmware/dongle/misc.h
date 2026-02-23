#ifndef MISC_H
#define MISC_H

#include <stdint.h>

#include "defs.h"

void reset_to_bootloader(void);
void bootloader_check(void);

// Functions for general use
void memsetx(__xdata void *s, uint8_t c, uint8_t n);
int8_t memcmpxx(__xdata const void * buf1, __xdata const void * buf2, uint8_t count);
void memcpyxx(__xdata void *dst, __xdata const void *src, uint8_t n);
void memcpycx(__xdata void *dst, __code const void *src, uint8_t n);

// Functions for use in ISR:s
void isr_memcpyxx(__xdata void *dst, __xdata const void *src, uint8_t n);
void isr_memcpycx(__xdata void *dst, __code const void *src, uint8_t n);

#endif
