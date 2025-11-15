#!/bin/bash

# This script formats the hard disk image and copies a test file to it.
# It requires sudo privileges to run.

set -e

HDD_IMAGE="build/main_hdd.img"
MOUNT_POINT="/mnt/mm8os_hdd"
TEST_FILE="test.txt"

# Check if running as root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (use sudo)"
  exit 1
fi

# --- Start: Robust Cleanup ---
# Ensure the mount point is not busy from a previous failed run
umount "$MOUNT_POINT" || true
losetup -d $(losetup -j "$HDD_IMAGE" | cut -d: -f1) 2>/dev/null || true
# --- End: Robust Cleanup ---

echo "Setting up loopback device for $HDD_IMAGE..."
LOOP_DEV=$(losetup -fP --show "$HDD_IMAGE")
if [ -z "$LOOP_DEV" ]; then
    echo "Failed to set up loopback device."
    exit 1
fi

echo "Creating FAT16 filesystem on $LOOP_DEV..."
# The --mbr=no flag is crucial. It tells mkfs.fat to format the entire device
# as a single volume without a partition table, making it a "superfloppy".
mkfs.fat -F 16 -n "MM8OS_HDD" --mbr=no "$LOOP_DEV"

echo "Writing custom bootloader to MBR..."
# 1. Write a JMP instruction at the start of the sector
printf '\xeb\x3c\x90' | dd of="$LOOP_DEV" bs=1 count=3 conv=notrunc
# 2. Write our bootloader code, skipping the BPB area.
#    bs=1 count=450 skip=62: Skips the 62-byte BPB area in our assembled file.
dd if="build/stage1.bin" of="$LOOP_DEV" bs=1 count=450 skip=62 seek=62 conv=notrunc

echo "Creating mount point $MOUNT_POINT..."
mkdir -p "$MOUNT_POINT"

echo "Mounting $LOOP_DEV to $MOUNT_POINT..."
mount "$LOOP_DEV" "$MOUNT_POINT"

echo "Copying OS files to the disk image..."
cp "build/src/bootloader/stage2/stage2.bin" "$MOUNT_POINT/STAGE2.BIN"
cp "build/src/kernel/kernel.bin" "$MOUNT_POINT/KERNEL.BIN"

echo "Unmounting and detaching loopback device..."
umount "$MOUNT_POINT"
losetup -d "$LOOP_DEV"

echo ""
echo "Hard disk image formatted successfully."
echo "You can now run './run.sh' to boot from the hard disk."