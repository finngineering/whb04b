#ifndef UART_H
#define UART_H

#include <stdint.h>

#include "defs.h"

extern volatile uint8_t uart0_tx_done;

void uart0_setup(void);
void uart0_block_tx_byte(uint8_t byte);

#endif
