#include <stdint.h>

#include "ch55x/ch554.h"

#include "defs.h"
#include "flash.h"
#include "usb.h"
#include "misc.h"
#include "crc.h"
#include "hw3000.h"
#include "timer.h"

#include "whb04b.h"

void whb04b_setup(__xdata struct whb04b_context *whb04b)
{
    // Read configuration/settings from dataflash
    dataflash_readbytes(10, &(whb04b->config), sizeof(whb04b->config));

    // Configure RF starting state
    whb04b->rf_state = WHB04B_RFSTATE_SEARCHING;
    whb04b->rf_channel = whb04b->config.id_low;
    hw3000_channel_change(whb04b->rf_channel);

    // Assume we will use feedrate override from start
    whb04b->override_spindle = 0;

    // Start timeout timers
    timeout_start(&whb04b->rf_search_timeout);
    timeout_start_max(&whb04b->rf_recv_timeout);
    timeout_start_max(&whb04b->usb_recv_timeout);
}

void whb04b_process(__xdata struct whb04b_context *whb04b)
{
    uint8_t ret;
    uint8_t send_reply = 0;

    // Forward data from computer to pendant
    ret = usb_get_report06(&whb04b->report06_usb, sizeof(whb04b->report06_usb));
    if(ret == sizeof(whb04b->report06_usb)) {
        // TODO: error checking...
        ret = whb04b_convert_report06(whb04b);
        if(ret != sizeof(whb04b->report04_usb)) {
            timeout_start(&whb04b->usb_recv_timeout);
        }
    }
    // TODO: Don't hardcode timeout
    if(timeout_elapsed(&whb04b->usb_recv_timeout, 20000)) {
        whb04b_invalidate_report06(whb04b);
    }
    whb04b_forward_report06(whb04b);

    // Forward data from pendant to computer
    ret = hw3000_rxdata_get(&whb04b->report04_rf, sizeof(whb04b->report04_rf));
    if(ret > 0) {
        // Always send a reply after we received data
        send_reply = 1;
    }
    if(ret == sizeof(whb04b->report04_rf)) {
        // Validate RF report and conver to USB report
        ret = whb04b_convert_report04(whb04b);
        if(ret == sizeof(whb04b->report04_rf)) {
            // We received a valid report!
            timeout_start(&whb04b->rf_recv_timeout);
        } else {
            // Not valid, but update timeout and zero jog counts
            whb04b_refresh_report04(whb04b);
        }
    } else {
        // Not data, but update timeout and zero jog counts
        whb04b_refresh_report04(whb04b);
    }
    whb04b_forward_report04(whb04b);

    // Send reply to pendant
    if(send_reply) {
        hw3000_txdata_send();
    }


    switch(whb04b->rf_state) {
    case WHB04B_RFSTATE_SEARCHING:
        // Have we received a valid message?
        // TODO: Don't hardcode timeout
        if(!timeout_elapsed(&whb04b->rf_recv_timeout, 20000)) {
            whb04b->rf_state = WHB04B_RFSTATE_COMMUNICATING;
            break;
        }

        // Toggle channel every 33 ms while searching
        if(timeout_elapsed(&whb04b->rf_search_timeout, 330)) {
            timeout_start(&whb04b->rf_search_timeout);
            if(whb04b->rf_channel == whb04b->config.id_low) {
                whb04b->rf_channel = (whb04b->config.id_low & 0x40) + 8;
            } else {
                whb04b->rf_channel = whb04b->config.id_low;
            }
            hw3000_channel_change(whb04b->rf_channel);
        }
        break;
    case WHB04B_RFSTATE_COMMUNICATING:
        // Go back to searching if long time since last message
        // TODO: Don't hardcode timeout
        if(timeout_elapsed(&whb04b->rf_recv_timeout, 20000)) {
            whb04b->rf_state = WHB04B_RFSTATE_SEARCHING;
            break;
        }
        break;
    default:
        break;
    }
}

uint8_t whb04b_convert_report06(__xdata struct whb04b_context *whb04b)
{
    __xdata struct whb04b_report06_usb *rep_usb;
    __xdata struct whb04b_report06_rf *rep_rf;

    rep_usb = &whb04b->report06_usb;
    rep_rf = &whb04b->report06_rf;
    if(rep_usb->magic != 0xfdfe) {
        return 0;
    }

    rep_rf->len = sizeof(*rep_rf);
    rep_rf->magic = 'W';
    rep_rf->channel = whb04b->rf_channel;
    rep_rf->id_low = whb04b->config.id_low;
    rep_rf->id_mid = whb04b->config.id_mid;
    rep_rf->id_high = whb04b->config.id_high;
    rep_rf->flags = rep_usb->flags;
    memcpyxx(&rep_rf->axis, &rep_usb->axis, sizeof(rep_rf->axis));
    if(whb04b->override_spindle == 0) {
        rep_rf->override = rep_usb->feed_rate;
    } else {
        rep_rf->override = rep_usb->spindle_speed;
    }

    rep_rf->checksum = crc8_nrsc_5_00(rep_rf, sizeof(*rep_rf) - 1);

    return sizeof(*rep_usb);
}

void whb04b_invalidate_report06(__xdata struct whb04b_context *whb04b)
{
    __xdata struct whb04b_report06_rf *rep_rf;

    rep_rf = &whb04b->report06_rf;

    rep_rf->len = sizeof(*rep_rf);
    rep_rf->magic = 'W';
    rep_rf->channel = whb04b->rf_channel;
    rep_rf->id_low = whb04b->config.id_low;
    rep_rf->id_mid = whb04b->config.id_mid;
    rep_rf->id_high = whb04b->config.id_high;

    // Light up the screen as well as possible
    rep_rf->mode = 0;
    rep_rf->unknown = 0;
    rep_rf->reset = 1;
    rep_rf->work_coord = 1;
    rep_rf->axis[0].integer = 8888;
    rep_rf->axis[0].decimal = 8888;
    rep_rf->axis[1].integer = 8888;
    rep_rf->axis[1].decimal = 8888;
    rep_rf->axis[2].integer = 8888;
    rep_rf->axis[2].decimal = 8888;
    rep_rf->override = 8888;

    rep_rf->checksum = crc8_nrsc_5_00(rep_rf, sizeof(*rep_rf) - 1);
}

uint8_t whb04b_forward_report06(__xdata struct whb04b_context *whb04b)
{
    return hw3000_txdata_set(&whb04b->report06_rf, sizeof(whb04b->report06_rf));
}

uint8_t whb04b_convert_report04(__xdata struct whb04b_context *whb04b)
{
    __xdata struct whb04b_report04_usb *rep_usb;
    __xdata const struct whb04b_report04_rf *rep_rf;
    uint8_t wday;

    rep_usb = &whb04b->report04_usb;
    rep_rf = &whb04b->report04_rf;

    if(rep_rf->len != sizeof(*rep_rf)) {
        return 0;
    }
    if(rep_rf->magic != 'W') {
        return 1;
    }
    if(rep_rf->id_low != whb04b->config.id_low) {
        return 3;
    }
    if(rep_rf->id_mid != whb04b->config.id_mid) {
        return 4;
    }
    if(rep_rf->id_high != whb04b->config.id_high) {
        return 5;
    }
    if(rep_rf->checksum != crc8_nrsc_5_00(rep_rf, sizeof(*rep_rf) - 1)) {
        return sizeof(*rep_rf) - 1;
    }

    rep_usb->report_id = 0x04;
    rep_usb->timer = timeout_update(&whb04b->rf_recv_timeout) >> 8;
    rep_usb->button1 = rep_rf->button1;
    rep_usb->button2 = rep_rf->button2;
    rep_usb->feed_switch = rep_rf->feed_switch;
    rep_usb->axis_switch = rep_rf->axis_switch;
    rep_usb->jog_delta = rep_rf->jog_delta;
    wday = whb04b->report06_usb.wday;
    rep_usb->checksum = (~rep_usb->timer + rep_usb->button1) ^ (wday | ~rep_usb->timer);

    // Update which override is in use based on buttons pressed
    // TODO: Should we simply ignore button2? Need to test how the pendant is reacting if two buttons pressed...
    // TODO: Or should we just accept a higher delay and let the LinuxCNC driver handle this?
    if(rep_rf->button1 == 0x04 || rep_rf->button1 == 0x05) {
        // Feed+ or Feed- button pressed
        whb04b->override_spindle = 0;
    } else if(rep_rf->button1 == 0x06 || rep_rf->button1 == 0x07) {
        // Spindle+ or Spindle- button pressed
        whb04b->override_spindle = 1;
    }

    return sizeof(*rep_rf);
}

void whb04b_refresh_report04(__xdata struct whb04b_context *whb04b)
{
    __xdata struct whb04b_report04_usb *rep_usb;
    uint8_t wday;

    rep_usb = &whb04b->report04_usb;

    rep_usb->report_id = 0x04;
    rep_usb->timer = timeout_update(&whb04b->rf_recv_timeout) >> 8;

    // We need to zero jog delta here. Otherwise jog count will keep changing on computer side
    rep_usb->jog_delta = 0;

    wday = whb04b->report06_usb.wday;
    rep_usb->checksum = (~rep_usb->timer + rep_usb->button1) ^ (wday | ~rep_usb->timer);
}

uint8_t whb04b_forward_report04(__xdata struct whb04b_context *whb04b)
{
    return usb_set_report04(&whb04b->report04_usb, sizeof(whb04b->report04_usb));
}
