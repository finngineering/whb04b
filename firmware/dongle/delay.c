#include <stdint.h>

#include "ch55x/ch554.h"
#include "delay.h"

// Check CH554 "SDK" for more details
void delay_us(uint16_t n)
{
#if FREQ_SYS != 12000000
#error Only 12MHz clock frequency is supported
#endif
	n >>= 1;
	while(n) {
		SAFE_MOD++;
		n--;
	}
}

void delay_ms(uint16_t n)
{
	while(n) {
		delay_us(1000);
		n--;
	}
}                                         
