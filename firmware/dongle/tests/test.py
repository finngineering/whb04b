import usb.core
import usb.control
import usb.util
import sys


if __name__ == "__main__":
    # find our device
    dev = usb.core.find(idVendor=0x10ce, idProduct=0xeb93)
    if dev is None:
        raise ValueError('Device not found')
    
    print(dev.configurations)

    # Read standard descriptors
    usb.control.get_descriptor(dev, 0xff, usb.util.DESC_TYPE_DEVICE, 0, 0)
    usb.control.get_descriptor(dev, 0xff, usb.util.DESC_TYPE_CONFIG, 0, 0)
    usb.control.get_descriptor(dev, 0xff, usb.util.DESC_TYPE_INTERFACE, 0, 0)
    usb.control.get_descriptor(dev, 0xff, usb.util.DESC_TYPE_ENDPOINT, 0, 0)

    # We return STALL to CLEAR_FEATURE
    try:
        usb.control.clear_feature(dev, usb.control.DEVICE_REMOTE_WAKEUP, None)
    except usb.core.USBError:
        pass
    else:
        raise ValueError('CLEAR_FEATURE suddenly supported???')

    # We return STALL to SET_FEATURE
    try:
        usb.control.set_feature(dev, usb.control.DEVICE_REMOTE_WAKEUP, None)
    except usb.core.USBError:
        pass
    else:
        raise ValueError('SET_FEATURE suddenly supported???')

    # GET_STATUS
    usb.control.get_status(dev)

    # GET_CONFIGURATION
    usb.control.get_configuration(dev)

    # Read HID & HID REPORT descriptors
    dev.ctrl_transfer(0x81, 0x06, 0x2100, 0x0000, 0x00ff)
    dev.ctrl_transfer(0x81, 0x06, 0x2200, 0x0000, 0x00ff)

    # HID SET_IDLE
    dev.ctrl_transfer(0x21, 0x0a, 0x4200, 0x0000, 0x0000)
