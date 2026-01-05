#!/bin/bash

# Usage: ./run.sh [--iso | --usb | --iso-usb] [--gdb]

QEMU_FLAGS="-d guest_errors,cpu_reset -D qemu.log -m 4096 -no-reboot -no-shutdown -monitor stdio"

if [ "$2" == "--gdb" ]; then
    echo "GDB Debugging Enabled. QEMU will pause at start."
    echo "Run in another terminal: gdb -x debug.gdb"
    QEMU_FLAGS="$QEMU_FLAGS -s -S"
fi

if [ "$1" == "--iso" ]; then
    # Boot from ISO, but attach the HDD image as the primary master disk.
    # This simulates a real PC with the OS CD in the drive and a hard disk installed.
    qemu-system-i386 -cdrom build/mm8os.iso -boot order=d -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk $QEMU_FLAGS
elif [ "$1" == "--usb" ]; then
    # Boot from the USB image (simulated as a hard drive).
    # We attach the data HDD as the secondary drive (hdb) to avoid conflict with the boot drive.
    # UPDATE: We attach the USB image as Primary Slave (index=1) but force boot from it.
    # This keeps the Main HDD at Primary Master (index=0) for the OS, while avoiding USB emulation quirks.
    qemu-system-i386 -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk \
                     -drive file=build/mm8os_usb.img,format=raw,if=none,id=usbdisk \
                     -device ide-hd,drive=usbdisk,bus=ide.0,unit=1,bootindex=0 \
                     $QEMU_FLAGS
elif [ "$1" == "--iso-usb" ]; then
    # Boot the ISO file as if it were a USB stick (Hard Disk emulation).
    # Verify it is a Hybrid ISO (Magic bytes 0x55 0xAA at offset 510)
    if ! dd if=build/mm8os.iso bs=1 skip=510 count=2 status=none | hexdump -e '1/1 "%02x"' | grep -q "55aa"; then
        echo "Error: build/mm8os.iso is NOT a Hybrid ISO (missing boot signature)."
        echo "It cannot be booted as a USB stick. Please install 'syslinux-utils' and run ./create_iso.sh again."
        exit 1
    fi
    # This tests the 'isohybrid' functionality.
    qemu-system-i386 -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk \
                     -drive file=build/mm8os.iso,format=raw,if=none,id=isousb \
                     -device ide-hd,drive=isousb,bus=ide.0,unit=1,bootindex=0 \
                     $QEMU_FLAGS
else
    # Boot directly from floppy image
    qemu-system-i386 -boot order=a -drive file=build/main_floppy.img,format=raw,if=floppy -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk $QEMU_FLAGS
fi