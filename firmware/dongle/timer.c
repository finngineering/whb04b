#include <stdint.h>
#include <limits.h>

#include "ch55x/ch554.h"

#include "defs.h"

#include "timer.h"

// NOTE: Timer 1 is used by UART0

// 0.1 ms ticks
static volatile uint16_t ticks;

void timer0_isr(void) __interrupt (INT_NO_TMR0)
{
    ticks++;
}

void timer0_setup(void)
{
    ticks = 0;

    // Timer 0 mode 2 (8 bit overload/timer)
    TMOD = (TMOD & 0xf0) | bT0_M1;
    // Use Fys/12 for clock rate
    T2MOD &= ~bT0_CLK;
    // Timer raw rate is 1MHz, "divide" by 100 to generate 0.1ms ticks
    TH0 = (uint8_t)(0xff - 99);
    TL0 = TH0;
    // Clear any interrupt flag and start timer
    TF0 = 0;
    TR0 = 1;
    // Enable interrupt
    ET0 = 1;
}

inline uint16_t timer0_ticks(void)
{
    uint16_t t;
    E_DIS = 1;
//    DEBUG_PIN = 1;
    t = ticks;
//    DEBUG_PIN = 0;
    E_DIS = 0;
    
    return t;
}

void timeout_start(timeout_t *timeout)
{
    timeout->start = timer0_ticks();
    timeout->elapsed = 0;
}

void timeout_start_max(timeout_t *timeout)
{
    timeout->start = timer0_ticks();
    timeout->elapsed = USHRT_MAX;
}

uint16_t timeout_update(timeout_t *timeout)
{
    uint16_t curticks = timer0_ticks();
    uint16_t diff = curticks - timeout->start;

    // Increase elapsed time if appropriate
    if(diff > timeout->elapsed) {
        timeout->elapsed = diff;
    }

    // Check if we have overflowed
    if(diff < timeout->elapsed) {
        timeout->elapsed = USHRT_MAX;
    }

    return timeout->elapsed;
}

uint8_t timeout_elapsed(timeout_t *timeout, uint16_t time)
{
    uint16_t elapsed = timeout_update(timeout);
    
    return elapsed > time;
}
