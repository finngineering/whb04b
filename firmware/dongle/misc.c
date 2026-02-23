#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"
#include "delay.h"
#include "misc.h"

void (*bootloader)(void) = (void (*)(void))0x3800;

void reset_to_bootloader(void)
{
	// Disable interrupts
	IE = 0;

	// Generate a USB reset
	USB_CTRL = bUC_RESET_SIE;
	delay_ms(20);

	// Set flag that we should jump to bootloader next start
	RESET_KEEP = 0x55;
	// Software reset
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xaa;
	GLOBAL_CFG |= bSW_RESET;

	// Wait for reset
	while(1);
}

void bootloader_check(void)
{
	// Check if we should jump to the bootloader
	if(RESET_KEEP == 0x55) {
		// Reset the flag and jump
		RESET_KEEP = 0;
		bootloader();
	}
}

void memsetx(__xdata void *s, uint8_t c, uint8_t n)
{
    char *dst = s;
    
    while(n--) {
        *dst++ = c;
    }
}

int8_t memcmpxx(__xdata const void * buf1, __xdata const void * buf2, uint8_t count)
{
	if (!count)
		return 0;

	while ( --count && *((char *)buf1) == *((char *)buf2) ) {
		buf1 = (char *)buf1 + 1;
		buf2 = (char *)buf2 + 1;
	}

	return( *((unsigned char *)buf1) - *((unsigned char *)buf2) );
}


void memcpycx(__xdata void *dst, __code const void *src, uint8_t n)
{
	char *d = dst;
	const char *s = src;

	while (n > 0) {
		*d++ = *s++;
        n -= 1;
    }
}

void memcpyxx(__xdata void *dst, __xdata const void *src, uint8_t n)
{
	char *d = dst;
	const char *s = src;

	while (n > 0) {
		*d++ = *s++;
        n -= 1;
    }
}

void isr_memcpyxx(__xdata void *dst, __xdata const void *src, uint8_t n)
{
	char *d = dst;
	const char *s = src;

	while (n > 0) {
		*d++ = *s++;
        n -= 1;
    }
}

void isr_memcpycx(__xdata void *dst, __code const void *src, uint8_t n)
{
	char *d = dst;
	const char *s = src;

	while (n > 0) {
		*d++ = *s++;
        n -= 1;
    }
}
