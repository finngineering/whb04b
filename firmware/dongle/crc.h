#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#include "defs.h"

uint8_t crc8_nrsc_5_00(__xdata const uint8_t *buffer, uint8_t len);

#endif
