#!/bin/bash

# Check if the floppy image exists.
if [ ! -f "build/main_floppy.img" ]; then
    echo "Error: Floppy image 'build/main_floppy.img' not found."
    echo "Run 'cmake --build build' to create it."
    exit 1
fi

qemu-system-i386 -boot order=a -drive file=build/main_floppy.img,format=raw,if=floppy