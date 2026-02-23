#include <stdint.h>

#include "ch55x/ch554.h"
#include "ch55x/ch554_usb.h"
#include "delay.h"

#include "defs.h"
#include "uart.h"
#include "usb.h"
#include "timer.h"
#include "spi.h"
#include "flash.h"
#include "hw3000.h"
#include "misc.h"
#include "whb04b.h"

// SDCC requires interrupt handles to be declared in the same file as main()
void usb_isr(void) __interrupt (INT_NO_USB);
void timer0_isr(void) __interrupt (INT_NO_TMR0);
void uart0_isr(void) __interrupt (INT_NO_UART0);

void clock_setup(void)
{
    // Enter safe mode
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xaa;
#if FREQ_SYS == 12000000
    CLOCK_CFG = bOSC_EN_INT | 4;
#else
    #error FREQ_SYS not supported
#endif
    // Terminate safe mode
    SAFE_MOD = 0;
}

void main(void) {
    __xdata struct whb04b_context whb04b;

    // Jump to bootloader if this was requested by user
    bootloader_check();

    // Setup pin P3.0 as push-pull output for debug if needed
    P3_DIR_PU |= _BV(0); // Output
    P3_MOD_OC &= ~_BV(0); // Push-pull
    DEBUG_PIN = 0;


    clock_setup();
    uart0_setup();
    usb_setup();
    timer0_setup();
    // Enable global interrupts now that all interrupt functions are setup
    EA = 1;
    spi_setup();
    hw3000_setup();
    whb04b_setup(&whb04b);

    // TODO: There should be a USB function to write dataflash instead of this hardcoded crap
    if(dataflash_read(10) == 0xff) {
        dataflash_write(10, 0x4d);
        dataflash_write(11, 0xf1);
        dataflash_write(12, 0x00);
        dataflash_write(13, 0x4d);
        dataflash_write(14, 0xf1);
        dataflash_write(15, 0x00);
        dataflash_write(16, 0xe8);
    }

    uart0_block_tx_byte('S');
    uart0_block_tx_byte('T');
    uart0_block_tx_byte('A');
    uart0_block_tx_byte('R');
    uart0_block_tx_byte('T');

    while (1) {
        whb04b_process(&whb04b);
        usb_process();
        hw3000_process();
    }
}
