#ifndef WHB04B_H
#define WHB04B_H

#include "ch55x/ch554.h"

#include "timer.h"
#include "defs.h"

struct whb04b_config {
    uint8_t id_low;
    uint8_t id_mid;
    uint8_t id_high;
    uint8_t id_low2;
    uint8_t id_mid2;
    uint8_t id_high2;
    uint8_t unknown1;
    uint8_t unknown2;
};

enum whb04b_rfstate {
    WHB04B_RFSTATE_SEARCHING,
    WHB04B_RFSTATE_COMMUNICATING
};

struct whb04b_axis {
    uint16_t integer;
    uint16_t decimal;
};

struct whb04b_report06_usb {
    uint16_t magic;
    uint8_t wday;
    uint8_t flags;
    struct whb04b_axis axis[3];
    uint16_t feed_rate;
    uint16_t spindle_speed;
};

struct whb04b_report06_rf {
    uint8_t len;
    uint8_t magic;
    uint8_t channel;
    uint8_t id_low;
    uint8_t id_mid;
    uint8_t id_high;
    uint8_t flags;
    struct whb04b_axis axis[3];
    uint16_t override; // This is either feedrate or spindle override
    uint8_t checksum;
};

struct whb04b_report04_usb {
    uint8_t report_id;
    uint8_t timer;
    uint8_t button1;
    uint8_t button2;
    uint8_t feed_switch;
    uint8_t axis_switch;
    int8_t jog_delta;
    uint8_t checksum;
};

struct whb04b_report04_rf {
    uint8_t len;
    uint8_t magic;
    uint8_t newdata; // TODO: maybe we should do something with this?
    uint8_t id_low;
    uint8_t id_mid;
    uint8_t id_high;
    uint8_t button1;
    uint8_t button2;
    uint8_t feed_switch;
    uint8_t axis_switch;
    int8_t jog_delta;
    uint8_t unknown; // TODO: Only ever seen this as 0x00, but maybe in some special case it's not(?)
    uint8_t checksum;
};

struct whb04b_context {
    struct whb04b_config config;
    enum whb04b_rfstate rf_state;
    uint8_t rf_channel;
    timeout_t rf_search_timeout;
    timeout_t rf_recv_timeout;
    struct whb04b_report06_usb report06_usb;
    struct whb04b_report06_rf report06_rf;
    struct whb04b_report04_usb report04_usb;
    struct whb04b_report04_rf report04_rf;
    uint8_t override_spindle;
};

void whb04b_setup(__xdata struct whb04b_context *whb04b);
void whb04b_process(__xdata struct whb04b_context *whb04b);
uint8_t whb04b_convert_report06(__xdata struct whb04b_context *whb04b);
uint8_t whb04b_forward_report06(__xdata struct whb04b_context *whb04b);
uint8_t whb04b_convert_report04(__xdata struct whb04b_context *whb04b);
void whb04b_refresh_report04(__xdata struct whb04b_context *whb04b);
uint8_t whb04b_forward_report04(__xdata struct whb04b_context *whb04b);


#endif
