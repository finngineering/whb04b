# Open source firmware for XHC WHB04B pendant
This is an effort to create open source firmware for the wireless XHC WHB04B(-6) pendants for CNC applications. The wireless version consists of the main pendant itself, and a small USB "dongle" that plugs into the computer. Using the factory firmware with the xhc-whb04b-6 LinuxCNC, the USB dongle disconnects after a while. With this firmware, it is the hope that these disconnects could be avoided. At the same time, it allows for some improvements over the original firmware. For instance proper timeout information in case the communication to the pendant is lost.

## Dongle firmware
At this time, only the dongle firmware is under development. The dongle essentially consists of a CH554 microcontroller and a HW3000 433 MHz RF module. The dongle firmware is located in the firmware/dongle folder. It is currently not ready for "production" use.

The firmware_readout tool inside the tools folder should be usable to provide a partial readout of the factory dongle firmware. This is possible because by trying to read an unused descriptor, it just keeps reading program memory from where it last left off. Unfortunately, it appears that reading past the bootloader memory causes the device to reset, so we cannot "wrap around" the 16-bit memory space to continue reading from address 0. In the factory firmware for my dongle, I can read normal program memory from address 0x2913 to 0x37ff, which is approximately 27% of the usable program memory space. The program instructions end at address 0x3408, which means I could read 21% of the firmware.

I have obtained the complete firmware through another process explained here:
https://github.com/finngineering/ch55x_firmware_extractor
So if somebody else does a partial readout, we can check if the partial readout matches my firmware. If it does, it's very likely that the whole firmware is the same.

## Pendant firmware
There is currently no real plans for developing an open source firmware for the pendant itself.

## Driver
I have had some thoughts about creating a different driver/component for LinuxCNC. Some of the USB functions were implemented and are available in the driver folder. This has been used for investigating the issues with the factory dongle firmware and as an aid for the open source firmware development. Currently, this driver knowns nothing about LinuxCNC.

## DISCLAIMER
NOT READY FOR PRODUCTION USE.