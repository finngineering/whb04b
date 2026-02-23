import usb.core
import usb.control
import usb.util
import sys
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Command and control for WHB04B open source firmware")
    parser.add_argument('-b', '--bootloader', action='store_true', default=False, help="Enter bootloader mode for programming")
    args = parser.parse_args()

    # find our device
    dev = usb.core.find(idVendor=0x10ce, idProduct=0xeb93)
    if dev is None:
        raise ValueError('Device not found')
    
    if dev.is_kernel_driver_active(0):
        dev.detach_kernel_driver(0)

    if args.bootloader:
        # HID SET_REPORT
        dev.ctrl_transfer(0x21, 0x09, 0x0000, 0x0000, b"\xff\xff\xff\xff\xff\xff\xff\xff")
