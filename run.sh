#!/bin/bash

# Usage: ./run.sh [--iso | --usb]

if [ "$1" == "--iso" ]; then
    # Boot from ISO, but attach the HDD image as the primary master disk.
    # This simulates a real PC with the OS CD in the drive and a hard disk installed.
    qemu-system-i386 -cdrom build/mm8os.iso -boot order=d -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk -d int,cpu_reset -D qemu.log -m 4096 -no-reboot
elif [ "$1" == "--usb" ]; then
    # Boot from the USB image (simulated as a hard drive).
    # We attach the data HDD as the secondary drive (hdb) to avoid conflict with the boot drive.
    # UPDATE: We attach the USB image as Primary Slave (index=1) but force boot from it.
    # This keeps the Main HDD at Primary Master (index=0) for the OS, while avoiding USB emulation quirks.
    qemu-system-i386 -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk \
                     -drive file=build/mm8os_usb.img,format=raw,if=none,id=usbdisk \
                     -device ide-hd,drive=usbdisk,bus=ide.0,unit=1,bootindex=0 \
                     -d int,cpu_reset -D qemu.log -m 4096 -no-reboot
else
    # Boot directly from floppy image
    qemu-system-i386 -boot order=a -drive file=build/main_floppy.img,format=raw,if=floppy -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk -d int,cpu_reset -D qemu.log -m 4096 -no-reboot
fi