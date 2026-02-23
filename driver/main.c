#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h> // TODO: Check if can be removed
#include <stdarg.h>

#include <unistd.h> // TODO: Remove, becuase only used for sleep()
#include <time.h> // TODO: check if can be removed
#include <string.h> // TODO: check if can be removed
#include <math.h> // Are we using?
#include <time.h> // Are we still using?

#include <libusb-1.0/libusb.h>

#define WHB04B_VENDOR 0x10ce
#define WHB04B_PRODUCT 0xeb93
#define WHB04B_INTERFACE 0
#define WHB04B_IN_ENDPOINT 0x81

enum whb04b_usb_state {
    STATE_OPEN_DEVICE,
    STATE_WORKING,
    STATE_RESTART,
};

struct whb04b_report_axis {
    uint16_t integer;
    uint16_t decimal;
} __attribute__((packed));

struct whb04b_report {
    uint16_t magic;
    uint8_t seed;
    union {
        struct {
            uint8_t mode:2;
            uint8_t unknown:4;
            uint8_t reset:1;
            uint8_t work_coord:1;
        };
        uint8_t flags;
    };
    struct whb04b_report_axis axes[3];
    uint16_t feedrate;
    uint16_t spindle_speed;
    uint32_t padding;
} __attribute__((packed));

struct whb04b_usb {
    struct libusb_context *context;
    struct libusb_device_handle *devh;
    struct libusb_transfer *int_transfer;
    struct libusb_transfer *control_transfer;
    uint8_t int_buffer[8]; // 8 bytes should be enough for WHB04B
    uint8_t control_buffer[LIBUSB_CONTROL_SETUP_SIZE + sizeof(struct whb04b_report)]; // TODO: only 16 bytes needed, as send 8 bytes at a time
    struct whb04b_report report; // TODO: Don't do it like this, please
    int report_offset; /// TODO: Don't do it like this, please
    int int_pending;
    int control_pending;
    
    enum whb04b_usb_state state;
};

void hexdump_data(char *buf, size_t bufsize, uint8_t *data, size_t datalen)
{
    size_t offset;

    if(bufsize == 0) {
        return;
    }

    for(offset = 0; offset < bufsize - 3; offset += 3) {
        if(!datalen) {
            break;
        }
        
        sprintf(buf + offset, "%02x ", *data);
        data++;
        datalen--;
    }
    buf[offset] = 0;
}

void infof(char *format, ...)
{
    va_list args;

    long ms;
    time_t s;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    s = ts.tv_sec;
    ms = round(ts.tv_nsec / 1.0e6);
    if(ms > 999) {
        s++;
        ms = 0;
    }

    printf("%ld.%03ld info: ", s, ms);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

void whb04b_update_report(struct whb04b_report *report)
{
    assert(report != 0);

    static uint8_t first_time = 1;
    if(first_time) {
        first_time = 0;
        report->feedrate = 120;
        report->spindle_speed = 80;
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    report->magic = 0xfdfe;
    report->seed = tm->tm_mday;
//    report->unknown = (uint8_t)t;

    report->axes[0].integer = tm->tm_year + 1900;
    report->axes[0].decimal = tm->tm_mon + 1;
    report->axes[1].integer = tm->tm_mday;
    report->axes[1].decimal = tm->tm_hour;
    report->axes[2].integer = tm->tm_min;
    report->axes[2].decimal = tm->tm_sec;

}

void usb_interrupt_callback(struct libusb_transfer *transfer)
{
    struct whb04b_usb *usb;
    static uint8_t indata_last[8] = {0};
    static uint8_t counts = 0;

    assert(transfer != 0);
    usb = transfer->user_data;
    assert(usb != 0);

    usb->int_pending = 0;

    // In case the transfer was cancelled (by us!), we are already in a restart state
    if(transfer->status == LIBUSB_TRANSFER_CANCELLED) {
        return;
    }

    // Silently ignore timeouts. We simply sumbit a new request
    if(transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
        return;
    }

    // Do we have an actual error situation that needs handling?
    if(transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        printf("%s: transfer status is %d\n", __func__, transfer->status);
        printf("%s: resetting device and trying again\n", __func__);
        // TODO: Actually reset the device and so on...

        usb->state = STATE_RESTART;
        return;
    }

    // Did we get some data?
    if(transfer->actual_length > 0) {
        char buffer[64];
        hexdump_data(buffer, sizeof(buffer), transfer->buffer, transfer->actual_length);
        if((indata_last[0] == transfer->buffer[0]) && (memcmp(indata_last + 2, transfer->buffer + 2, 5) == 0)) {
            // In case only the timeout and checksum were updated, print it on the same line as last time
            printf("\033[F");
        }
        memcpy(indata_last, transfer->buffer, sizeof(indata_last));
        counts += transfer->buffer[6];
        infof("IN data: %s    %02x\n", buffer, counts);
        // TODO: Don't do it like this
        whb04b_update_report(&usb->report);
        if(transfer->buffer[2] == 0x0e) {
            usb->report.mode = 0;
        }
        if(transfer->buffer[2] == 0x0f) {
            usb->report.mode = 1;
        }
/*        if(transfer->buffer[2] == 0x10) {
            usb->report.mode = 2;
        }
        if(transfer->buffer[2] == 0x0d) {
            usb->report.mode = 3;
        }
*/        if(transfer->buffer[2] == 0x01) {
            usb->report.reset ^= 1;
        }
        if(transfer->buffer[2] == 0x04) {
            usb->report.feedrate += 5;
        }
        if(transfer->buffer[2] == 0x05) {
            usb->report.feedrate -= 5;
        }
        if(transfer->buffer[2] == 0x06) {
            usb->report.spindle_speed += 5;
        }
        if(transfer->buffer[2] == 0x07) {
            usb->report.spindle_speed -= 5;
        }

        // printf("report: ");
        // for(size_t i = 0; i < sizeof(usb->report); i++) {
        //     printf("%02x ", *((uint8_t*)(&usb->report)+i));
        // }
        // printf("\n");

        // Update the display next
        usb->report_offset = 0;
    }
}

void usb_control_callback(struct libusb_transfer *transfer)
{
    struct whb04b_usb *usb;

    assert(transfer != 0);
    usb = transfer->user_data;
    assert(usb != 0);

    usb->control_pending = 0;

    // In case the transfer was cancelled (by us!), we are already in a restart state
    if(transfer->status == LIBUSB_TRANSFER_CANCELLED) {
        return;
    }

    // Do we have an actual error situation that needs handling?
    if(transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        printf("%s: transfer status is %d\n", __func__, transfer->status);
        printf("%s: resetting device and trying again\n", __func__);
        // TODO: Actually reset the device and so on...

        usb->state = STATE_RESTART;
        return;
    }

    usb->report_offset += 7;
}

int usb_init(struct whb04b_usb *usb)
{
    int r;

    assert(usb != 0);

    memset(usb, 0, sizeof(*usb));

    r = libusb_init_context(&usb->context, NULL, 0);
    usb->devh = NULL;
    usb->int_transfer = libusb_alloc_transfer(0);
    assert(usb->int_transfer != NULL); // TODO: Maybe not assert in here...
    usb->control_transfer = libusb_alloc_transfer(0);
    assert(usb->control_transfer != NULL); // TODO: same as above
    usb->state = STATE_OPEN_DEVICE;
    usb->int_pending = 0;
    usb->control_pending = 0;
    usb->report_offset = 24;

    return r;
}

int usb_process(struct whb04b_usb *usb)
{
    int r;

    assert(usb != 0);

    switch(usb->state) {
    case STATE_OPEN_DEVICE:
        assert(usb->devh == NULL);
        usb->devh = libusb_open_device_with_vid_pid(usb->context, WHB04B_VENDOR, WHB04B_PRODUCT);
        infof("usb->devh: %p\n", usb->devh); // TODO: remove
        if(NULL == usb->devh) {
            break;
        }
        
        // Detach kernel driver from interface 0 (the only interface for WHB04B)
        r = libusb_detach_kernel_driver(usb->devh, WHB04B_INTERFACE);
        if(r != 0 && r != LIBUSB_ERROR_NOT_FOUND) {
            printf("%s: libusb_detach_kernel_driver failed with code %d = %s\n", __func__, r, libusb_error_name(r));
            libusb_close(usb->devh);
            usb->devh = NULL;
            break;
        }

        // Claim interface 0
        r = libusb_claim_interface(usb->devh, WHB04B_INTERFACE);
        if(r != 0) {
            printf("%s: libusb_claim_interface failed with code %d = %s\n", __func__, r, libusb_error_name(r));
            // Give the device back to the kernel driver
            libusb_attach_kernel_driver(usb->devh, WHB04B_INTERFACE);
            libusb_close(usb->devh);
            usb->devh = NULL;
            break;
        }

        // Create the interrupt transfer
        libusb_fill_interrupt_transfer( usb->int_transfer,
                                        usb->devh,
                                        WHB04B_IN_ENDPOINT,
                                        usb->int_buffer,
                                        sizeof(usb->int_buffer),
                                        usb_interrupt_callback,
                                        usb,
                                        1500); // TODO: don't hardcode timeout

        r = libusb_control_transfer(    usb->devh,
                                        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                        0x0a, // We don't have a define for SET_IDLE
                                        0x1900, // 0x19 * 4 ms = 100 ms timeout
                                        0,
                                        NULL,
                                        0,
                                        1000);
        if(r < 0) {
            printf("%s: libusb_control_transfer (SET_IDLE) failed with code %d = %s\n", __func__, r, libusb_error_name(r));
            printf("%s: this is not critical, so we continue anyway\n", __func__);            
        }

        // Everything is now setup, so go to working state
        usb->state = STATE_WORKING;
        break;
    
    case STATE_WORKING:
        // Check if we are currently "idling"
        if(!usb->int_pending && usb->report_offset > 14) {
            // Submit a new interrupt transfer
            r = libusb_submit_transfer(usb->int_transfer);
            if(r != 0) {
                printf("%s: libusb_submit_transfer (interrupt) failed with code %d = %s\n", __func__, r, libusb_error_name(r));
                printf("%s: cleaning up device and starting anew\n", __func__);
                // TODO: actually clean up and start anew
                break;
            }
            usb->int_pending = 1;
        }

        // Check if we should submit a SET_REPORT to update the display
        if(!usb->control_pending && usb->report_offset <= 14) {
            libusb_fill_control_setup(  usb->control_buffer,
                                        LIBUSB_DT_HID, // TODO: Not sure if this is the correct define even if the value is correct
                                        0x9, // SET_REPORT
                                        0x0306, // 0x03 = Feature, 0x06 = Report ID
                                        0x00, //  Interface = WHB04B_INTERFACE ?
                                        8);
            
            // One report frame is one static byte 0x06 followed by 7 bytes of data
            *(usb->control_buffer + LIBUSB_CONTROL_SETUP_SIZE) = 0x06; // Constant first byte
            memcpy(usb->control_buffer + LIBUSB_CONTROL_SETUP_SIZE + 1, (char*)&usb->report + usb->report_offset, 7);
            libusb_fill_control_transfer(   usb->control_transfer,
                                            usb->devh,
                                            usb->control_buffer,
                                            usb_control_callback,
                                            usb,
                                            500); // TODO: don't hardcode timeout
//            printf("submitting report offset %d\n", usb->report_offset);
            r = libusb_submit_transfer(usb->control_transfer);
            if(r != 0) {
                printf("%s: libusb_submit_transfer (control) failed with code %d = %s\n", __func__, r, libusb_error_name(r));
                printf("%s: cleaning up device and starting anew\n", __func__);
                // TODO: actually clean up and start anew
                break;
            }
            usb->control_pending = 1;
        }
        break;
    
    case STATE_RESTART:
        if(usb->int_pending) {
            // Cancel ongoing transfer and wait for the callback to have been called
            r = libusb_cancel_transfer(usb->int_transfer);
            if(r != 0) {
                printf("%s: libusb_cancel_transfer failed with code %d = %s\n", __func__, r, libusb_error_name(r));
            }
            break;
        }
        r = libusb_release_interface(usb->devh, WHB04B_INTERFACE);
        if(r != 0) {
            printf("%s: libusb_release_interface failed with code %d = %s\n", __func__, r, libusb_error_name(r));
        }
        r = libusb_attach_kernel_driver(usb->devh, WHB04B_INTERFACE);
        if(r != 0) {
            printf("%s: libusb_attach_kernel_driver failed with code %d = %s\n", __func__, r, libusb_error_name(r));
        }
        libusb_close(usb->devh);
        usb->devh = NULL;
        usb->state = STATE_OPEN_DEVICE;
        break;

    default:
        break;
    }

    struct timeval timeout = {0};
    timeout.tv_usec = 100*1000; // TODO: let this remain at zero
    r = libusb_handle_events_timeout_completed(usb->context, &timeout, NULL);
    if(r != 0) {
        // We shouldn't really ever end up here
        printf("%s: libusb_handle_events_timeout_completed failed with code %d = %s\n", __func__, r, libusb_error_name(r));
        printf("%s: cleaning up device and starting anew\n", __func__);
    }

    return 0;
}

void usb_exit(struct whb04b_usb *usb)
{
    assert(usb != 0);
    // TODO: close all open devices first

    libusb_exit(usb->context);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    struct whb04b_usb whb04b_usb;
    int r;

    r = usb_init(&whb04b_usb);
    if(r < 0) {
        printf("usb_init failed with code %d. This is unrecoverable, so exiting program\n", r);
        exit(r);
    }

    while(1) {
        usb_process(&whb04b_usb);

//        struct timespec timeout = {};
//        timeout.tv_nsec = 100*1000*1000;

//        nanosleep(&timeout, NULL);
    }

    usb_exit(&whb04b_usb);
}
