#ifndef TIMER_H
#define TIMER_H

#include "defs.h"

typedef struct {
    uint16_t start;
    uint16_t elapsed;
} timeout_t;

// NOTE: Timer 1 is used by UART0
void timer0_setup(void);
inline uint16_t timer0_ticks(void);

void timeout_start(timeout_t *timeout);
void timeout_start_max(timeout_t *timeout);
uint16_t timeout_update(timeout_t *timeout);
uint8_t timeout_elapsed(timeout_t *timeout, uint16_t time);

#endif
