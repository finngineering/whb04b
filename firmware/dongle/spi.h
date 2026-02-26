#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"

#define SPI_BIT_DELAY 1

#define SPI_CSN_PIN 4
#define SPI_CLK_PIN 7
#define SPI_MOSI_PIN 5
#define SPI_MISO_PIN 6

SBIT(SPI_CSN,  0x90, SPI_CSN_PIN); // P1.4
SBIT(SPI_CLK,  0x90, SPI_CLK_PIN); // P1.7
SBIT(SPI_MOSI, 0x90, SPI_MOSI_PIN); // P1.5
SBIT(SPI_MISO, 0x90, SPI_MISO_PIN); // P1.6

void spi_setup(void);
void spi_write16(uint8_t addr, uint16_t val);
uint16_t spi_read16(uint8_t addr);
void spi_write8(uint8_t addr, uint8_t val);
uint8_t spi_read8(uint8_t addr);
void spi_writebytes(uint8_t addr, __xdata const uint8_t *buffer, uint8_t len);
void spi_readbytes(uint8_t addr, __xdata uint8_t *buffer, uint8_t len);

#endif
