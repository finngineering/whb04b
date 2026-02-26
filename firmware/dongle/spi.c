#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"
#include "delay.h"
#include "spi.h"


void spi_setup(void)
{
    // Setup chip select (negative) pin
    P1_DIR_PU |= _BV(SPI_CSN_PIN); // Output
    P1_MOD_OC &= ~_BV(SPI_CSN_PIN); // Push-pull
    SPI_CSN = 1; // Drive pin high (no select)

    // Setup SPI clock pin
    P1_DIR_PU |= _BV(SPI_CLK_PIN); // Output
    P1_MOD_OC &= ~_BV(SPI_CLK_PIN); // Push-pull
    SPI_CLK = 0; // Clock rest state is low

    // Setup MOSI pin
    P1_DIR_PU |= _BV(SPI_MOSI_PIN); // Output
    P1_MOD_OC &= ~_BV(SPI_MOSI_PIN); // Push-pull
    SPI_MOSI = 0;

    // Setup MISO pin
    P1_DIR_PU &= ~_BV(SPI_MISO_PIN); // Input
    P1_MOD_OC &= ~_BV(SPI_MISO_PIN); // Push-pull
}

void spi_write16(uint8_t addr, uint16_t val)
{
    // MSB high to indicat write operation
    addr |= 0x80;

    SPI_CSN = 0;

    // Write address
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((addr & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        addr <<= 1;
    }

    // Write value
    for(uint8_t i = 0; i < 16; i++) {
        SPI_CLK = 1;
        if((val & 0x8000) == 0x8000) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        val <<= 1;        
    }

    SPI_CSN = 1;
}

uint16_t spi_read16(uint8_t addr)
{
    uint16_t val = 0;

    // MSB low to indicate read operation
    addr &= ~0x80;

    SPI_CSN = 0;

    // Write address
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((addr & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        addr <<= 1;
    }

    // Read value
    for(uint8_t i = 0; i < 16; i++) {
        val <<= 1;
        SPI_CLK = 1;
        delay_us(SPI_BIT_DELAY);
        SPI_CLK = 0;

        if(SPI_MISO) {
            val += 1;
        } 
    }

    SPI_CSN = 1;

    return val;
}

void spi_write8(uint8_t addr, uint8_t val)
{
    // MSB high to indicat write operation
    addr |= 0x80;

    SPI_CSN = 0;

    // Write address
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((addr & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        addr <<= 1;
    }

    // Write value
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((val & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        val <<= 1;        
    }

    SPI_CSN = 1;
}

uint8_t spi_read8(uint8_t addr)
{
    uint8_t val = 0;

    // MSB low to indicate read operation
    addr &= ~0x80;

    SPI_CSN = 0;

    // Write address
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((addr & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        addr <<= 1;
    }

    // Read value
    for(uint8_t i = 0; i < 8; i++) {
        val <<= 1;        
        SPI_CLK = 1;
        delay_us(SPI_BIT_DELAY);
        SPI_CLK = 0;

        if(SPI_MISO) {
            val += 1;
        } 
    }

    SPI_CSN = 1;

    return val;
}

void spi_writebytes(uint8_t addr, __xdata const uint8_t *buffer, uint8_t len)
{
    // MSB high to indicat write operation
    addr |= 0x80;

    SPI_CSN = 0;

    // Write address
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((addr & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        addr <<= 1;
    }

    for(uint8_t bytenum = 0; bytenum < len; bytenum++) {
        // Write value
        uint8_t val = buffer[bytenum];
        for(uint8_t i = 0; i < 8; i++) {
            SPI_CLK = 1;
            if((val & 0x80) == 0x80) {
                SPI_MOSI = 1;
            } else {
                SPI_MOSI = 0;
            }
            SPI_CLK = 0;
            val <<= 1;        
        }
    }

    SPI_CSN = 1;
}

void spi_readbytes(uint8_t addr, __xdata uint8_t *buffer, uint8_t len)
{
    uint8_t val = 0;

    // MSB low to indicate read operation
    addr &= ~0x80;

    SPI_CSN = 0;

    // Write address
    for(uint8_t i = 0; i < 8; i++) {
        SPI_CLK = 1;
        if((addr & 0x80) == 0x80) {
            SPI_MOSI = 1;
        } else {
            SPI_MOSI = 0;
        }
        SPI_CLK = 0;
        addr <<= 1;
    }

    for(uint8_t bytenum = 0; bytenum < len; bytenum++) {
        // Read value
        for(uint8_t i = 0; i < 8; i++) {
            val <<= 1;        
            SPI_CLK = 1;
            delay_us(SPI_BIT_DELAY);
            SPI_CLK = 0;

            if(SPI_MISO) {
                val += 1;
            } 
        }
        buffer[bytenum] = val;
    }
    SPI_CSN = 1;
}
