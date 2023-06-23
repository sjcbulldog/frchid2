#!/bin/bash

INSTPROG=installer/xerofrchid-1.0.0.exe
CM0PROG=fw/bootloader_cm0p/build/APP_CY8CPROTO-062-4343W/Debug/bootloader_cm0p.hex
CM4PROG=fw/frchid/build/BOOT/APP_CY8CPROTO-062-4343W/Debug/frchid.hex

if [ ! -f $INSTPROG ]; then
    echo "Build the FRC  HID Bootloader program for window"
    exit 1
fi
cp $INSTPROG release

if [ ! -f $CM0PROG ]; then
    echo "Build the bootloader_cm0p firmware program"
    exit 1 ;
fi
cp $CM0PROG release

if [ ! -f $CM4PROG ]; then
    echo "Build the frchid firmware program"
    exit 1 ;
fi
cp $CM4PROG release
