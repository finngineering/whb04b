#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"

#include "flash.h"

uint8_t dataflash_read(uint8_t offset)
{
    // Dataflash data at even bytes only, and only low data byte is valid
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    ROM_ADDR_L = offset << 1;

    ROM_CTRL = ROM_CMD_READ;

    return ROM_DATA_L;
}

uint8_t dataflash_readbytes(uint8_t offset, __xdata uint8_t *buffer, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++) {
        buffer[i] = dataflash_read(offset + i);
    }

    return len;
}

void dataflash_write(uint8_t offset, uint8_t data)
{
    // Enable writing data to dataflash
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xaa;
    GLOBAL_CFG |= bDATA_WE;

    // Write the data
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    ROM_ADDR_L = offset << 1;
    ROM_DATA_L = data;
    ROM_CTRL = ROM_CMD_WRITE;

    // Disable dataflash writing
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xaa;
    GLOBAL_CFG &=  ~bDATA_WE;
    SAFE_MOD = 0;
}
