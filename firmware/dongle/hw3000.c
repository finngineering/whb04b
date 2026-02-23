#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"
#include "spi.h"
#include "delay.h"
#include "timer.h"
#include "misc.h"

#include "hw3000.h"

static enum hw3000_state state;

static uint16_t cal_ticks = 0;
static uint16_t tx_ticks = 0;

static __xdata uint8_t tx_buffer[32] = {0};
static uint8_t tx_buffer_len = 0;
static __xdata uint8_t rx_buffer[32] = {0};
static uint8_t rx_buffer_len = 0;
static uint8_t hw3000_channel = 0;

struct hw3000_command {
    uint8_t addr;
    uint16_t value;
};

__code struct hw3000_command hw3000_init_commands[] =  {
    // These are written to BANK1, so we don't have register names
    {0x1b, 0x1371},
    {0x1c, 0x1351},
    {0x1d, 0x0b51},
    {0x1e, 0x2f71},
    {0x1f, 0x2f51},
    {0x20, 0x2e51},
    {0x21, 0x2e31},
    {0x22, 0x4b31},
    {0x23, 0x4731},
    {0x24, 0x4631},
    {0x25, 0x4531},
    {0x26, 0x4431},
    {0x27, 0x6131},
    {0x28, 0x6031},
    {0x29, 0x6011},
    {0x2a, 0x6009},
};

void hw3000_setup(void)
{
    uint16_t val;

    // Setup HW3000 power down pin
    P1_DIR_PU |= _BV(HW3000_PDN_PIN); // Output
    P1_MOD_OC &= ~_BV(HW3000_PDN_PIN); // Push-pull

    // Setup HW3000 IRQN pin
    P3_DIR_PU &= ~_BV(HW3000_IRQN_PIN); // Input
    P3_MOD_OC &= ~_BV(HW3000_IRQN_PIN); // Push-pull

    // Power cycle HW3000
    hw3000_power(0);
    delay_ms(5);
    hw3000_power(1);
    delay_ms(5);

    // Wait for max 1 s for chip to become available
    spi_write16(HW3000_REG_BANKSEL, 0x5555);
    for(uint8_t i = 0; i < 100; i++) {
        val = spi_read16(HW3000_REG_INT);
        // Wait for ready after power on
        if((val & 0x4000) == 0x4000) {
            break;
        }
        delay_ms(10);
    }

    // Write calibration constants to BANK1
    spi_write16(HW3000_REG_BANKSEL, 0x55aa);
    for(uint8_t i = 0; i < sizeof(hw3000_init_commands)/sizeof(hw3000_init_commands[0]); i++) {
        spi_write16(hw3000_init_commands[i].addr, hw3000_init_commands[i].value);
    }
    spi_write16(0x03, 0x0508);
    spi_write16(0x11, 0xc630);
#ifdef HW3000_RATE_38400
    spi_write16(0x14, 0x1915);
#else
#error Unsupported HW3000 symbol rate
#endif
#ifdef HW3000_BAND_433MHZ
    spi_write16(0x17, 0xf6c2);
#else
#error Unsupported HW3000 frequency band
#endif
    spi_write16(0x51, 0x001b);
    spi_write16(0x55, 0x8003);
    spi_write16(0x56, 0x4155);
    spi_write16(0x62, 0x70ed);

    // Go back to BANK0
    spi_write16(HW3000_REG_BANKSEL, 0x5555);

    spi_write16(HW3000_REG_RXPHR1, 0x8000);
    spi_write16(HW3000_REG_IFSET, 0x2e35);
    spi_write16(HW3000_REG_MODECTRL, 0x100f);
#ifdef HW3000_OSC_20MHZ
    spi_write16(HW3000_REG_MODEMCTRL, 0x5201);
#else
#error Unsupported HW3000 oscillator frequency
#endif
#ifdef HW3000_FREQ_DIRECT
    hw3000_freq_set();
#else
#error Unsupported HW3000 frequency mode
#endif
    hw3000_rate_set();
    hw3000_power_set();
    hw3000_p0addr0_set(0x5425);
    hw3000_frame_set();

    hw3000_rx_disable();

    // TODO: Not sure if we need to set this here...
    hw3000_channel_set(hw3000_channel);

    state = HW3000_STATE_IDLE;
    hw3000_rx_enable();

    hw3000_calibrate();
    cal_ticks = timer0_ticks();
    tx_ticks = cal_ticks;
    memsetx(tx_buffer, 0, sizeof(tx_buffer));
    tx_buffer_len = 0;
    memsetx(rx_buffer, 0, sizeof(rx_buffer));
    rx_buffer_len = 0;
}

void hw3000_freq_set(void)
{
// We only support 20MHz oscillator and 38400Hz symbol rate
#ifndef HW3000_OSC_20MHZ
#error Unsupported HW3000 oscillator frequency
#endif

#ifndef HW3000_RATE_38400
#error Unsupported HW3000 symbol rate
#endif

    spi_write16(HW3000_REG_RFCFG, 0x3312);
    spi_write16(HW3000_REG_FREQCFG1, 0x8013);
    spi_write16(HW3000_REG_FREQCFG2, 0x3333);
    spi_write16(HW3000_REG_FREQCFG0, 0x2056);
}

void hw3000_rate_set(void)
{
// We only support 20MHz oscillator and 38400Hz symbol rate
#ifndef HW3000_OSC_20MHZ
#error Unsupported HW3000 oscillator frequency
#endif

#ifndef HW3000_RATE_38400
#error Unsupported HW3000 symbol rate
#endif

    spi_write16(HW3000_REG_SYMRATE0, 0x013a);
    spi_write16(HW3000_REG_SYMRATE1, 0x0093);
    spi_write16(HW3000_REG_FILTERBAND, 0x0033);
    spi_write16(HW3000_REG_DEVIATION, 0x0052);
}

void hw3000_power_set(void)
{
    // There are some other options for this, but a bit difficult to dechiper
    spi_write16(HW3000_REG_PACFG, 0xff3f);
}

void hw3000_p0addr0_set(uint16_t addr)
{
    spi_write16(HW3000_REG_P0ADDR0, addr);
}

void hw3000_frame_set(void)
{
#ifndef HW3000_MODE_FIFO
#error Unsupported HW3000 frame mode
#endif

    spi_write16(HW3000_REG_PKTCTRL, 0x0100);
    spi_write16(HW3000_REG_GPIOCFG0, 0xc009);
    spi_write16(HW3000_REG_GPIOCFG1, 0x08c0);
    spi_write16(HW3000_REG_FIFOTHRES, 0x8080);
    spi_write16(HW3000_REG_PKTCFG0, 0x4008);
    spi_write16(HW3000_REG_PREACFG, 0x0206);
    spi_write16(HW3000_REG_FIFOSTA, 0x0000);
    spi_write16(HW3000_REG_LEN0RXADD, 0x0000);
    spi_write16(HW3000_REG_PIPECTRL, 0x0001);
}

int8_t hw3000_rx_enable(void)
{
    if(state == HW3000_STATE_TX) {
        return -1;
    }

    if(state != HW3000_STATE_IDLE) {
        hw3000_rx_disable();
        // TODO: check this delay and also one before returning
        delay_ms(5);
    }

    spi_write16(HW3000_REG_LEN0PKLEN, 0x000d);
    spi_write16(HW3000_REG_TRCTRL, 0x0080);
    spi_write16(HW3000_REG_FIFOSTA, 0x0200);

    state = HW3000_STATE_RX;

    return 0;
}

void hw3000_rx_disable(void)
{
    spi_write16(HW3000_REG_TRCTRL, 0x0000);
    spi_write16(HW3000_REG_RXPHR1, 0x0001);
    state = HW3000_STATE_IDLE;
}

int8_t hw3000_calibrate(void)
{
    uint16_t val;

    spi_write16(HW3000_REG_RCCFG1, 0x002b);
    for(uint8_t i = 0; i < 50; i++) {
        val = spi_read16(HW3000_REG_RCSTA);
        if((val & 0x8000) == 0x8000) {
            return 0;
        }
        // TODO: check delay is reasonable
        delay_ms(1);
    }
    return -1;
}

void hw3000_channel_set(uint8_t channel)
{
    spi_write16(HW3000_REG_CHCFG0, (uint16_t)(channel & 0x3f) << 8);
}

uint8_t hw3000_fifo_read(__xdata uint8_t *buffer, uint8_t maxlen)
{
    uint8_t len;

    len = spi_read8(HW3000_REG_FIFODATA);
    if(len > maxlen) {
        len = maxlen;
    }

    // Store at least length
    if(maxlen > 0) {
        buffer[0] = len;
    }

    // Read data only if there is something to read
    if(len > 0) {
        buffer[0] = len;
        spi_readbytes(HW3000_REG_FIFODATA, buffer + 1, len - 1);
    }

    return len;
}

void hw3000_fifo_write(__xdata uint8_t *buffer, uint8_t len)
{
    spi_writebytes(HW3000_REG_FIFODATA, buffer, len);
}

void hw3000_fifo_tx(__xdata uint8_t *buffer, uint8_t len)
{
    if(state != HW3000_STATE_TX) {
        if(state != HW3000_STATE_IDLE) {
            hw3000_rx_disable();
            delay_us(150);
        }

        spi_write16(HW3000_REG_TRCTRL, 0x0100);
        spi_write16(HW3000_REG_FIFOSTA, 0x0100);
        delay_us(150);
        spi_write16(HW3000_REG_LEN0PKLEN, 0x0016);
        hw3000_fifo_write(buffer, len);
        // Send message
        spi_write16(HW3000_REG_FIFOCTRL, 0x0001);

        // Wait for send completion or timeout
        uint16_t t_start = timer0_ticks();
        while(1) {
            if(timer0_ticks() - t_start > 1000) {
                break;
            }
            if(HW3000_IRQN == 0) {
                break;
            }
        }
        spi_write16(HW3000_REG_FIFOCTRL, 0x0000);

        // Same as disable_rx
        spi_write16(HW3000_REG_RXPHR1, 0x0001);
        spi_write16(HW3000_REG_TRCTRL, 0x0000);

        state = HW3000_STATE_IDLE;
    }
}

// This performs channel change AND calibration
void hw3000_channel_change(uint8_t channel)
{
    hw3000_channel = channel;

    hw3000_rx_disable();
    delay_ms(1);
    hw3000_channel_set(channel);
    hw3000_rx_enable();

    delay_us(150);
    hw3000_calibrate();
    cal_ticks = timer0_ticks();
}

void hw3000_power(uint8_t on)
{
    HW3000_PDN = on != 1;
}

void hw3000_process(void)
{
    if(HW3000_IRQN == 0) {
        // We should have some data in FIFO...
        rx_buffer_len = hw3000_fifo_read(rx_buffer, sizeof(rx_buffer));

        // Sending data back is initiated from whb04b, because we need to take which buttons was pushed into account
    }
}

uint8_t hw3000_txdata_set(__xdata uint8_t *buffer, uint8_t len)
{
    if(len > sizeof(tx_buffer)) {
        len = sizeof(tx_buffer);
    }
    memcpyxx(tx_buffer, buffer, len);
    tx_buffer_len = len;

    return len;
}

uint8_t hw3000_rxdata_get(__xdata uint8_t *buffer, uint8_t maxlen)
{
    uint8_t len = rx_buffer_len;
    if(len > maxlen) {
        len = maxlen;
    }

    memcpyxx(buffer, rx_buffer, len);

    // Ensure we don't return the same data twice (unless we have received it again)
    rx_buffer_len = 0;

    return len;
}

void hw3000_txdata_send(void)
{
    hw3000_fifo_tx(tx_buffer, tx_buffer_len);
    hw3000_channel_change(hw3000_channel);
}
