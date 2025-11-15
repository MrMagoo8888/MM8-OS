#!/bin/bash

# Check if the HDD image appears to be formatted by looking for the boot signature.
if ! hexdump -s 510 -n 2 -e '"%04x"' build/main_hdd.img | grep -q "aa55"; then
    echo "Warning: HDD image 'build/main_hdd.img' does not appear to be formatted."
    echo "Run 'cmake --build build --target format_hdd' to format it (requires sudo)."
fi

# The first line is commented out as it causes a "write lock" error.
# We will use the second line, which boots from a floppy and uses a separate hard disk image.
qemu-system-i386 -boot order=a -drive file=build/main_floppy.img,format=raw,if=floppy -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk