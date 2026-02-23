#ifndef USB_H
#define USB_H

#include <stdint.h>

#include "ch55x/ch554_usb.h"
#include "defs.h"

void usb_setup(void);
void usb_reset(void);
void usb_process(void);
uint8_t usb_get_report06(__xdata uint8_t *buffer, uint8_t maxlen);
uint8_t usb_set_report04(__xdata uint8_t *buffer, uint8_t len);

#endif
