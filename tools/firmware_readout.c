#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <libusb-1.0/libusb.h>

#define WHB04B_VENDOR 0x10ce
#define WHB04B_PRODUCT 0xeb93

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

int main(int argc, char *argv[])
{
    struct libusb_context *context;
    struct libusb_device_handle *devh;
    struct libusb_device_descriptor desc_dev;
    int r;
    uint8_t firmware[USHRT_MAX];
    uint32_t readbytes = 0;
    uint8_t bootloader_magic[] = {0x12, 0x01, 0x10, 0x01, 0xff, 0x80, 0x55, 0x08, 0x48, 0x43, 0xe0, 0x55};
    uint32_t bootloader_start = 0;

    if(argc < 2) {
        printf("usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Init libusb
    r = libusb_init_context(&context, NULL, 0);
    if(r != LIBUSB_SUCCESS) {
        printf("libusb_init_context failed with error %d = %s\n", r, libusb_strerror(r));
        return EXIT_FAILURE;
    }

    // Open device
    devh = libusb_open_device_with_vid_pid(context, WHB04B_VENDOR, WHB04B_PRODUCT);
    if(NULL == devh) {
        printf("libusb_open_device_with_vid_pid could not open device\n");
        return EXIT_FAILURE;
    }

    // Read device descriptor
    r = libusb_control_transfer(devh,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
        LIBUSB_REQUEST_GET_DESCRIPTOR,
        LIBUSB_DT_DEVICE << 8,
        0,
        (unsigned char*)&desc_dev,
        sizeof(desc_dev),
        1000);
    if(r != sizeof(desc_dev)) {
        printf("libusb_control_transfer failed to read device descriptor with error %d = %s\n", r, libusb_strerror(r));
        return EXIT_FAILURE;
    }

    infof("Read device descriptor with idVendor=0x%04x and idProduct=0x%04x\n", desc_dev.idVendor, desc_dev.idProduct);
    if(desc_dev.idVendor != WHB04B_VENDOR || desc_dev.idProduct != WHB04B_PRODUCT) {
        printf("Vendor and or product not matching expectation. Bailing out.\n");
        return EXIT_FAILURE;
    }
    
    memset(firmware, 0, sizeof(firmware));
    infof("Starting firmware readout process\n\n");
    while(readbytes < USHRT_MAX) {
        printf("\033[F"); // Go back one line...
        infof("Read %5d of %5d bytes = %6.2f%%\n", readbytes, USHRT_MAX, (float)readbytes/USHRT_MAX);

        // Read in small chunks so we don't go past the bootloader unnoticed
        int readlen = USHRT_MAX - readbytes;
        if(readlen > 8) {
            readlen = 8;
        }
        // Read non-existant descriptor, which simply continues reading memory from the last address
        r = libusb_control_transfer(devh,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
            LIBUSB_REQUEST_GET_DESCRIPTOR,
            0x42 << 8, // Any descriptor that isn't available in the dongle works
            0,
            firmware + readbytes,
            readlen,
            1000);
        if(r < 0) {
            printf("libusb_control_transfer read failed with error %d = %s\n", r, libusb_strerror(r));
            break;
        }
        if(r != readlen) {
            printf("libusb_control_transfer unexpected bytes read: %d instead of %d\n\n", r, readlen);
        }

        // Look for bootloader magic
        uint8_t *addr = memmem(firmware, readbytes, bootloader_magic, sizeof(bootloader_magic));
        if(NULL != addr) {
            uint32_t desc_offset = addr - firmware;
            // We need at least 2 bytes more to determine the bootloader version
            if(readbytes - desc_offset > 2) {
                uint8_t bootloader_minor = firmware[desc_offset + sizeof(bootloader_magic)];
                uint8_t bootloader_major = firmware[desc_offset + sizeof(bootloader_magic) + 1];
                // Version is BCD encoded, so convert to normal integers
                bootloader_minor = (bootloader_minor >> 4)*10 + (bootloader_minor & 0xf);
                bootloader_major = (bootloader_major >> 4)*10 + (bootloader_major & 0xf);
                infof("Found bootloader version %d.%02d device descriptor after %d bytes\n",
                    bootloader_major, bootloader_minor, desc_offset);

                // Calculate bootloader start address (offset) based on known bootloaders
                if(bootloader_major == 2 && bootloader_minor == 40) {
                    bootloader_start = desc_offset - 0x68e;
                } else if(bootloader_major == 2 && bootloader_minor == 50) {
                    bootloader_start = desc_offset - 0x690;
                } else {
                    bootloader_start = 0;
                }
                break;
            }
        }
        readbytes += r;
    }

    if(bootloader_start > 0) {
        uint32_t firmware_end = 0x37ff;
        uint32_t firmware_start = firmware_end - bootloader_start + 1;
        infof("Based on bootloader address, read firmware is from address 0x%04x to 0x%04x\n",
            firmware_start, firmware_end);
    }

    FILE *fp = fopen(argv[1], "wb");
    if(NULL == fp) {
        printf("Could not open file %s for writing\n", argv[1]);
    } else {
        if(fwrite(firmware, 1, readbytes, fp) == readbytes) {
            infof("Wrote %d bytes to file %s\n", readbytes, argv[1]);
        } else {
            printf("Could not write %d bytes to file %s\n", readbytes, argv[1]);
        }
        fclose(fp);
    }

    libusb_close(devh);
    libusb_exit(context);

    return EXIT_SUCCESS;
}
