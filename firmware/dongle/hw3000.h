#ifndef HW3000_H
#define HW3000_H

#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"

#define HW3000_REG_TRCTRL           0x01
#define HW3000_REG_PKTCTRL          0x02
#define HW3000_REG_PKTCFG0          0x03
#define HW3000_REG_FIFOTHRES        0x06
#define HW3000_REG_FIFOCTRL         0x0c
#define HW3000_REG_LEN0PKLEN        0x0d
#define HW3000_REG_FIFOSTA          0x0e
#define HW3000_REG_INT              0x0f
#define HW3000_REG_RXPHR1           0x11
#define HW3000_REG_PIPECTRL         0x13
#define HW3000_REG_P0ADDR0          0x14
#define HW3000_REG_MODECTRL         0x1c
#define HW3000_REG_GPIOCFG0         0x1d
#define HW3000_REG_GPIOCFG1         0x1e
#define HW3000_REG_PREACFG          0x24
#define HW3000_REG_MODEMCTRL        0x25
#define HW3000_REG_IFSET            0x26
#define HW3000_REG_CHCFG0           0x28
#define HW3000_REG_DEVIATION        0x2c
#define HW3000_REG_FILTERBAND       0x2e
#define HW3000_REG_FREQCFG0         0x2f
#define HW3000_REG_FREQCFG1         0x30
#define HW3000_REG_FREQCFG2         0x31
#define HW3000_REG_SYMRATE0         0x32
#define HW3000_REG_SYMRATE1         0x33
#define HW3000_REG_RFCFG            0x35
#define HW3000_REG_CHIPSTA1         0x36
#define HW3000_REG_RCSTA            0x39
#define HW3000_REG_RCCFG1           0x3a
#define HW3000_REG_PACFG            0x40
#define HW3000_REG_BANKSEL          0x4c
#define HW3000_REG_LEN0RXADD        0x4e
#define HW3000_REG_FIFODATA         0x70

enum hw3000_state {
    HW3000_STATE_POWER_DOWN,
    HW3000_STATE_DEEP_SLEEP,
    HW3000_STATE_SLEEP,
    HW3000_STATE_IDLE,
    HW3000_STATE_TX,
    HW3000_STATE_RX
};


// Defines for configuration of HW3000. If selection/define not supported, compilation intentionally fails
#define HW3000_RATE_38400
#define HW3000_BAND_433MHZ
#define HW3000_OSC_20MHZ
#define HW3000_FREQ_DIRECT
#define HW3000_MODE_FIFO


#define HW3000_PDN_PIN 1
SBIT(HW3000_PDN,  0x90, HW3000_PDN_PIN); // P1.1
#define HW3000_IRQN_PIN 3
SBIT(HW3000_IRQN, 0xb0, HW3000_IRQN_PIN);

void hw3000_setup(void);
void hw3000_freq_set(void);
void hw3000_rate_set(void);
void hw3000_power_set(void);
void hw3000_p0addr0_set(uint16_t addr);
void hw3000_frame_set(void);
int8_t hw3000_rx_enable(void);
void hw3000_rx_disable(void);
int8_t hw3000_calibrate(void);
void hw3000_channel_set(uint8_t channel);
uint8_t hw3000_fifo_read(__xdata uint8_t *buffer, uint8_t maxlen);
void hw3000_fifo_write(__xdata uint8_t *buffer, uint8_t len);
void hw3000_fifo_tx(__xdata uint8_t *buffer, uint8_t len);
void hw3000_channel_change(uint8_t channel);

void hw3000_power(uint8_t on);

void hw3000_process(void);
uint8_t hw3000_txdata_set(__xdata uint8_t *buffer, uint8_t len);
uint8_t hw3000_rxdata_get(__xdata uint8_t *buffer, uint8_t maxlen);
void hw3000_txdata_send(void);

#endif
