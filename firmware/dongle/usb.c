
#include "ch55x/ch554.h"
#include "ch55x/ch554_usb.h"

#include "defs.h"
#include "delay.h"
#include "misc.h"
#include "whb04b.h"
#include "timer.h"

#include "usb.h"

#define ID_VENDOR 0x10ce
#define ID_PRODUCT 0xeb93

typedef struct _WHB_USB_DESC {
    USB_DEV_DESCR desc_dev;
    USB_CFG_DESCR desc_config;
    USB_ITF_DESCR desc_intf;
    USB_HID_DESCR desc_hid;
    USB_ENDP_DESCR desc_endp;
} WHB_USB_DESC;

__code WHB_USB_DESC usb_desc = {
    .desc_dev = {
        .bLength = sizeof(USB_DEV_DESCR),
        .bDescriptorType = USB_DESCR_TYP_DEVICE,
        .bcdUSBL = 0x00,
        .bcdUSBH = 0x02,
        .bDeviceClass = USB_DEV_CLASS_RESERVED,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = DEFAULT_ENDP0_SIZE,
        .idVendorL = (uint8_t)ID_VENDOR,
        .idVendorH = (uint8_t)(ID_VENDOR >> 8),
        .idProductL = (uint8_t)ID_PRODUCT,
        .idProductH = (uint8_t)(ID_PRODUCT >> 8),
        .bcdDeviceL = 0, // TODO: Add FW version
        .bcdDeviceH = 0, // TODO: Add FW version
        .iManufacturer = 0,
        .iProduct = 0,
        .iSerialNumber = 0,
        .bNumConfigurations = 1
    },
    .desc_config = {
        .bLength = sizeof(USB_CFG_DESCR),
        .bDescriptorType = USB_DESCR_TYP_CONFIG,
        .wTotalLengthL = sizeof(WHB_USB_DESC) - sizeof(USB_DEV_DESCR),
        .wTotalLengthH = (sizeof(WHB_USB_DESC) - sizeof(USB_DEV_DESCR)) >> 8,
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = 0xA0,
        .MaxPower = 0x32
    },
    .desc_intf = {
        .bLength = sizeof(USB_ITF_DESCR),
        .bDescriptorType = USB_DESCR_TYP_INTERF,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_DEV_CLASS_HID,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0 //5 // TODO: This was 5. What should it be?
    },
    .desc_hid = {
        .bLength = sizeof(USB_HID_DESCR),
        .bDescriptorType = USB_DESCR_TYP_HID,
        .bcdHIDL = 0,
        .bcdHIDH = 1,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
        .bDescriptorTypeX = 0x22,
        .wDescriptorLengthL = 0x2e, // TODO: Don't hardcode
        .wDescriptorLengthH = 0
    },
    .desc_endp = {
        .bLength = sizeof(USB_ENDP_DESCR),
        .bDescriptorType = USB_DESCR_TYP_ENDP,
        .bEndpointAddress = 0x81,
        .bmAttributes = 0x3, // Interrupt endpoint
        .wMaxPacketSizeL = 8,
        .wMaxPacketSizeH = 0,
        .bInterval = 2
    }
};

// TODO: This is straight from WHB04B-6. Not sure if it makes any sense.
__code uint8_t usb_desc_report[] = {
    0x06, 0x00, 0xff, 0x09,
    0x01, 0xa1, 0x01, 0x85,
    0x04, 0x09, 0x01, 0x15,
    0x00, 0x26, 0x00, 0xff,
    0x75, 0x08, 0x95, 0x07,
    0x81, 0x02, 0xc0, 0x06,
    0x00, 0xff, 0x09, 0x01,
    0xa1, 0x01, 0x85, 0x06,
    0x09, 0x01, 0x15, 0x00,
    0x26, 0x00, 0xff, 0x75,
    0x08, 0x95, 0x07, 0xb1,
    0x06, 0xc0
};

// Initialization needed for SDCC to reserve the XSEG space
__xdata __at(0x0000) volatile uint8_t USB_EP0_BUF[64] = {0};
__xdata __at(0x0040) volatile uint8_t USB_EP1_BUF[64] = {0};

__xdata USB_SETUP_REQ USB_EP0_SETUP;
__xdata volatile uint8_t USB_HID_REPORT06[24] = {0};
volatile uint8_t USB_HID_REPORT06_OFFSET = 0;
volatile uint8_t USB_HID_REPORT06_READY = 0;
__xdata struct whb04b_report04_usb USB_HID_REPORT04 = {0};


volatile uint8_t USB_SEND_EP1 = 0;

volatile uint16_t USB_IDLE_SETTING = 5000;
__xdata volatile uint16_t USB_PROCESS_TICKS = 0;
volatile uint8_t USB_CONFIGURATION = 0;

// Forward declarations
void usb_handle_EP0(void);
void usb_handle_EP1(void);

// INFO: It appears that no interrupts are generated in case endpoint control is set to NAK or STALL,
// i.e. UEP1_CTRL = UEP_R_RES_NAK. The exception is SETUP packets on EP0 (and perhaps others?)

void usb_setup(void)
{
    __code uint8_t report06_init[] = {0xfe, 0xfd, 0x01, 0xc2};

    memsetx(USB_EP0_BUF, 0, sizeof(USB_EP0_BUF));
    memsetx(USB_EP1_BUF, 0, sizeof(USB_EP1_BUF));
    memsetx(USB_HID_REPORT06, 0, sizeof(USB_HID_REPORT06));
    memsetx(&USB_HID_REPORT04, 0, sizeof(USB_HID_REPORT04));

    memcpycx(USB_HID_REPORT06, report06_init, sizeof(report06_init));
    USB_HID_REPORT06_OFFSET = 0;

    // Enable USB functions
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // 1.5k PU, NAK after transfer, DMA enable

    // Setup DMA
    UEP0_DMA = (uint16_t)USB_EP0_BUF;
    UEP1_DMA = (uint16_t)USB_EP1_BUF;

    // Enable EP1 transmit
    UEP4_1_MOD = bUEP1_TX_EN;

    delay_ms(20);
    usb_reset();

    USB_PROCESS_TICKS = timer0_ticks();
    USB_SEND_EP1 = 0;
}

void usb_reset(void)
{
    uint8_t tmp;

    tmp = USB_CTRL;
    USB_CTRL = bUC_RESET_SIE;
    delay_ms(20);
    USB_CTRL = tmp;

    // Enable physical port
    UDEV_CTRL = bUD_PD_DIS | bUD_PORT_EN; // Disable PD resistors, enable USB port
    // Device address 0
    USB_DEV_AD = 0;

    // Setup EP0
    UEP0_CTRL = UEP_R_RES_NAK | UEP_T_RES_NAK;
    UEP0_T_LEN = 0;
    // Setup EP1. NAK by default
    UEP1_CTRL = UEP_R_RES_NAK | UEP_T_RES_NAK;
    UEP1_T_LEN = 0;

    // Reset potential interrupt flags
    USB_INT_FG = 0xff;
    // Enable USB interrupts
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;

}

void usb_process(void)
{
    uint16_t ticks;

    // Ticks are in 0.1 ms
    ticks = timer0_ticks();

    // Check if we should send a report based on timeout
    if(USB_IDLE_SETTING != 0) {
        if(ticks - USB_PROCESS_TICKS > USB_IDLE_SETTING) {
            USB_SEND_EP1 = 1;
        }
    }

    E_DIS = 1;
    // Check if we should send data
    if(USB_SEND_EP1 == 1) {
        USB_PROCESS_TICKS = ticks;

        // In case a transfer is already in progress, wait for it to complete before sending
        if(UEP1_T_LEN == 0) {
            // Copy report to endpoint buffer
            memcpyxx(USB_EP1_BUF, &USB_HID_REPORT04, sizeof(USB_HID_REPORT04));

            USB_SEND_EP1 = 0;
            // Enable EP1 transmitter (will be disable in interrupt after transfer)
            UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
            UEP1_T_LEN = 8;
        }
    }
    E_DIS = 0;
}

uint8_t usb_get_report06(__xdata uint8_t *buffer, uint8_t maxlen)
{
    uint16_t start_ticks;

    uint8_t len = sizeof(USB_HID_REPORT06);
    if(len > maxlen) {
        len = maxlen;
    }

    // We require offset to be at zero so we don't copy in the middle of an update
    start_ticks = timer0_ticks();
    while(timer0_ticks() - start_ticks < 10) {
        // Check if offset is at zero
        if(USB_HID_REPORT06_OFFSET != 0) {
            continue;
        }
        // Check again after disabling interrupts
        E_DIS = 1;
        if(USB_HID_REPORT06_OFFSET != 0) {
            E_DIS = 0;
            continue;
        }
        // Now we are safe to copy the report...
        memcpyxx(buffer, USB_HID_REPORT06, len);
        E_DIS = 0;
        return len;
    }

    return 0;
}

uint8_t usb_set_report04(__xdata uint8_t *buffer, uint8_t len)
{
    if(len > sizeof(USB_HID_REPORT04)) {
        len = sizeof(USB_HID_REPORT04);
    }

    if(memcmpxx(&USB_HID_REPORT04, buffer, len) != 0) {
        USB_SEND_EP1 = 1;
    }
    memcpyxx(&USB_HID_REPORT04, buffer, len);

    return len;
}

void usb_isr(void) __interrupt (INT_NO_USB)
{
    // If we have a control transfer, we cancel previous response states quickly. This prevents
    // e.g. a previous STALL from being sent to the current (handled) control transfer
    if((USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) == (UIS_TOKEN_SETUP | 0)) {
        UEP0_CTRL = UEP_R_RES_NAK | UEP_T_RES_NAK;
    }

    if(UIF_TRANSFER) {

        switch(USB_INT_ST & MASK_UIS_ENDP) {
        case 0:
            usb_handle_EP0();
            break;
        case 1:
            usb_handle_EP1();
            break;
        default:
            break;
        }

        UIF_TRANSFER = 0;
    }
    if(UIF_SUSPEND) {
        // TODO: Not sure if there is much we can do here...
        UIF_SUSPEND = 0;
    }
    if(UIF_BUS_RST) {
        // TODO: This needs to be handled. Use usb_reset but without forcing the reset
        UIF_BUS_RST = 0;

        UEP0_CTRL = UEP_R_RES_NAK | UEP_T_RES_NAK;
    }
}

void usb_handle_EP0(void)
{
    __code static uint8_t *txPtr = 0;
    static uint8_t txLen = 0;
    uint8_t descLen;
    uint8_t curLen;

    uint8_t token = USB_INT_ST & MASK_UIS_TOKEN;
    if((token == UIS_TOKEN_SETUP) && (USB_RX_LEN == 8)) {
        // TODO: Maybe some form of error handling in case USB_RX_LEN != 8?
        
        // Store the SETUP packet for later use
        isr_memcpyxx(&USB_EP0_SETUP, USB_EP0_BUF, sizeof(USB_EP0_SETUP));

        // TODO: Now all standard requests are lumped together, regardless of recipient. Is that okay?
        if((USB_EP0_SETUP.bRequestType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD) {
            // Standard request
            switch(USB_EP0_SETUP.bRequest) {
            case USB_GET_STATUS:
                // Since we don't implement any of these features, always return two empty bytes
                USB_EP0_BUF[0] = 0;
                USB_EP0_BUF[1] = 0;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                UEP0_T_LEN = 2;
                break;
            case USB_GET_DESCRIPTOR:
                descLen = 0;
                // Only allow reading descriptor number 0, but we have no other ones
                if(USB_EP0_SETUP.wValueL != 0) {
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_STALL;
                    break;
                }

                // Handle requests based on recipient
                if((USB_EP0_SETUP.bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
                    if(USB_EP0_SETUP.wValueH == USB_DESCR_TYP_DEVICE) {
                        txPtr = &(usb_desc.desc_dev);
                        descLen = sizeof(usb_desc.desc_dev);
                    } else if(USB_EP0_SETUP.wValueH == USB_DESCR_TYP_CONFIG) {
                        txPtr = &(usb_desc.desc_config);
                        // Allow to read the whole configuration hierarchy
                        descLen = sizeof(usb_desc) - sizeof(usb_desc.desc_dev);
                    } else if(USB_EP0_SETUP.wValueH == USB_DESCR_TYP_INTERF) {
                        txPtr = &(usb_desc.desc_intf);
                        descLen = sizeof(usb_desc.desc_intf);
                    } else if(USB_EP0_SETUP.wValueH == USB_DESCR_TYP_ENDP) {
                        txPtr = &(usb_desc.desc_endp);
                        descLen = sizeof(usb_desc.desc_endp);
                    }
                } else if((USB_EP0_SETUP.bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_INTERF) {
                    if(USB_EP0_SETUP.wValueH == USB_DESCR_TYP_HID) {
                        txPtr = &(usb_desc.desc_hid);
                        descLen = sizeof(usb_desc.desc_hid);
                    } else if(USB_EP0_SETUP.wValueH == USB_DESCR_TYP_REPORT) {
                        txPtr = usb_desc_report;
                        descLen = sizeof(usb_desc_report);
                    }
                }
                

                if(descLen > 0) {
                    // We have actual data to send. Prepare stuff
                    curLen = descLen > DEFAULT_ENDP0_SIZE? DEFAULT_ENDP0_SIZE : descLen;
                    // TODO: can we disregard wLengthH here? probably
                    if(curLen > USB_EP0_SETUP.wLengthL) {
                        curLen = USB_EP0_SETUP.wLengthL;
                    }
                    isr_memcpycx(USB_EP0_BUF, txPtr, curLen);
                    txPtr += curLen;
                    // TODO: can we disregard wLengthH here? probably
                    if(descLen <= USB_EP0_SETUP.wLengthL) {
                        txLen = descLen - curLen;
                    } else {
                        txLen = USB_EP0_SETUP.wLengthL - curLen;
                    }

                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                    UEP0_T_LEN = curLen;
                } else {
                    // Stall!
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_STALL;
                }
                break;
            case USB_SET_ADDRESS:
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                UEP0_T_LEN = 0;
                break;
            case USB_GET_CONFIGURATION:
                USB_EP0_BUF[0] = USB_CONFIGURATION;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                UEP0_T_LEN = 1;
                break;
            case USB_SET_CONFIGURATION:
                UEP0_T_LEN = 0;
                // Only valid configuration values for us are 0 and 1
                if(USB_EP0_SETUP.wValueL == 0 || USB_EP0_SETUP.wValueL == 1) {
                    USB_CONFIGURATION = USB_EP0_SETUP.wValueL;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                } else {
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_STALL;
                }
                break;
            case USB_CLEAR_FEATURE:
            case USB_SET_FEATURE:
            case USB_SET_INTERFACE:
            case USB_GET_INTERFACE:
            case USB_SYNCH_FRAME:
                // We explicitly don't support these requests...
                // TODO: Technically, the ENDPOINT_HALT should be implemented for bulk and int endpoints...
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_STALL;
                UEP0_T_LEN = 0;
                break;
            default:
                // Stall for unhandled requests
                if((USB_EP0_SETUP.bRequestType & USB_REQ_TYP_IN) == USB_REQ_TYP_IN) {
                    // Stall on IN packets
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_T_RES_STALL;
                } else {
                    // Stall on OUT packets
//                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL;
                }
                break;
            }
            
        } else if(USB_EP0_SETUP.bRequestType == (USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)) {
            // HID
            if(USB_EP0_SETUP.bRequest == HID_SET_IDLE) {
                if(USB_EP0_SETUP.wValueH == 0) {
                    USB_IDLE_SETTING = 0;
                } else {
                    // Setting from wValueH is in 4 ms increments (40 ticks)
                    // We need to avoid multiplication in ISR, so times 32 has to be close enough
                    USB_IDLE_SETTING = USB_EP0_SETUP.wValueH << 5;
                }
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_NAK | UEP_T_RES_ACK;
                UEP0_T_LEN = 0;
            } else if(USB_EP0_SETUP.bRequest == HID_SET_REPORT) {
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                UEP0_T_LEN = 0;
          }
        } else {
            // Non-standard requests
            UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
        }
    } else if(token == UIS_TOKEN_IN) {
        // Send data to host...
        if(UEP0_T_LEN != 0) {
            UEP0_T_LEN = 0;
        }
        if((USB_EP0_SETUP.bRequestType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD) {
            // Process standard request, any recipient, IN
            // TODO: Should split this by recipient as well

            if(USB_EP0_SETUP.bRequest == USB_GET_DESCRIPTOR) {

                curLen = txLen > 8? 8 : txLen;
                isr_memcpycx(USB_EP0_BUF, txPtr, curLen);
                txPtr += curLen;
                txLen -= curLen;

                UEP0_CTRL ^= bUEP_T_TOG;
                UEP0_T_LEN = curLen;
            } else if(USB_EP0_SETUP.bRequest == USB_SET_ADDRESS) {
                USB_DEV_AD = USB_EP0_SETUP.wValueL;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
            } else {
                // Go back to normal state
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
            }
        } else {
            // Go back to normal state
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        }
    } else if(token == UIS_TOKEN_OUT) {
        // Receive data from host
        if((USB_EP0_SETUP.bRequestType == (USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)) &&
            (USB_EP0_SETUP.bRequest == HID_SET_REPORT)) {
            // USB HID SET_REPORT
            switch(USB_EP0_BUF[0]) {
            case 0x06:
                // Ensure we have the correct magic if this is the first part of the report
                if(USB_HID_REPORT06_OFFSET == 0) {
                    USB_HID_REPORT06_READY = 0;
                    if(USB_EP0_BUF[1] != 0xfe || USB_EP0_BUF[2] != 0xfd) {
                        break;
                    }
                }

                // Copy from EP0 buffer to report 06
                if(USB_HID_REPORT06_OFFSET + 7 <= sizeof(USB_HID_REPORT06)) {
                    isr_memcpyxx(USB_HID_REPORT06 + USB_HID_REPORT06_OFFSET, USB_EP0_BUF + 1, 7);
                    USB_HID_REPORT06_OFFSET += 7;
                    if(USB_HID_REPORT06_OFFSET >= 21) {
                        USB_HID_REPORT06_OFFSET = 0;
                        USB_HID_REPORT06_READY = 1;
                    }
                }
                
                break;
            case 0xff:
                reset_to_bootloader();
                break;
            default:
                break;
            };
            UEP0_CTRL = bUEP_T_TOG | UEP_R_RES_NAK | UEP_T_RES_ACK;;
            UEP0_T_LEN = 0;
        }

    }
}

void usb_handle_EP1(void)
{
    uint8_t token = USB_INT_ST & MASK_UIS_TOKEN;
    if(token == UIS_TOKEN_IN) {
        // NAK on EP1 IN
        UEP1_CTRL = (UEP1_CTRL ^ bUEP_T_TOG) | UEP_R_RES_NAK | UEP_T_RES_NAK;
        UEP1_T_LEN = 0;
    }
}
