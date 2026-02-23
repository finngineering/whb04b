#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"

uint8_t dataflash_read(uint8_t offset);
uint8_t dataflash_readbytes(uint8_t offset, __xdata uint8_t *buffer, uint8_t len);
void dataflash_write(uint8_t offset, uint8_t data);

#endif
