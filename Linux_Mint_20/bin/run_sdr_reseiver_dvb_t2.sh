#! /bin/bash

modprobe - r sdr_msi3101
modprobe - r msi001
modprobe - r msi2500

SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="2500",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3000",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3010",MODE:="0666"
SUBSYSTEM=="usb",ENV{DEVTYPE}=="usb_device",ATTRS{idVendor}=="1df7",ATTRS{idProduct}=="3020",MODE:="0666"

LD_LIBRARY_PATH=. ./sdr_receiver_dvb_t2 $@
