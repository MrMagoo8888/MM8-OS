#!/bin/bash

# This script creates a bootable ISO image from the floppy and HDD images.
# It uses El Torito floppy emulation to boot the floppy image.
# The HDD image is included as a file on the ISO.

set -e

BUILD_DIR="build"
ISO_DIR="${BUILD_DIR}/iso"
OUTPUT_ISO="${BUILD_DIR}/mm8os.iso"
FLOPPY_IMG="${BUILD_DIR}/main_floppy.img"
HDD_IMG="${BUILD_DIR}/main_hdd.img"

# Check dependencies
if command -v mkisofs >/dev/null 2>&1; then
    MKISOFS="mkisofs"
elif command -v genisoimage >/dev/null 2>&1; then
    MKISOFS="genisoimage"
else
    echo "Error: 'mkisofs' or 'genisoimage' not found. Please install cdrtools or genisoimage."
    exit 1
fi

# Prepare staging directory
rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR"

# Check if source images exist
if [ ! -f "$FLOPPY_IMG" ]; then
    echo "Error: $FLOPPY_IMG not found. Please build the project first."
    exit 1
fi

echo "Copying images..."
cp "$FLOPPY_IMG" "$ISO_DIR/"

if [ -f "$HDD_IMG" ]; then
    echo "Including HDD image as a file on the ISO..."
    cp "$HDD_IMG" "$ISO_DIR/"
fi

# Create ISO
echo "Generating ISO..."
# -b: Boot image (relative to ISO root)
# -c: Boot catalog (created by mkisofs)
# -J: Joliet (Windows compat)
# -R: Rock Ridge (Unix permissions)
$MKISOFS -J -R -b $(basename "$FLOPPY_IMG") \
    -c boot.catalog \
    -o "$OUTPUT_ISO" \
    "$ISO_DIR"

# Note: 'isohybrid' is not compatible with El Torito floppy emulation.
# The resulting ISO is bootable on CD/DVD and VirtualBox/QEMU.
# To boot from a USB stick on real hardware, it is often easier to write
# the 'main_floppy.img' directly to the USB stick (e.g. using dd or Rufus).
echo "Success! ISO created at $OUTPUT_ISO"