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

echo "Setting up loopback device for $HDD_IMAGE..."
LOOP_DEV=$(losetup -fP --show "$HDD_IMAGE")
if [ -z "$LOOP_DEV" ]; then
    echo "Failed to set up loopback device."
    exit 1
fi

echo "Creating FAT12 filesystem on $LOOP_DEV..."
# We use --mbr=y to write a Master Boot Record (MBR) to the disk image.
# This creates a partition table and the 0xAA55 boot signature, which is required for the disk to be recognized as a valid, formatted drive.
mkfs.fat -F 12 --mbr=y "$LOOP_DEV"

echo "Creating mount point $MOUNT_POINT..."
mkdir -p "$MOUNT_POINT"

echo "Mounting $LOOP_DEV to $MOUNT_POINT..."
mount "$LOOP_DEV" "$MOUNT_POINT"

echo "Copying $TEST_FILE to the disk image..."
cp "$TEST_FILE" "$MOUNT_POINT/"

echo "Unmounting and detaching loopback device..."
umount "$MOUNT_POINT"
losetup -d "$LOOP_DEV"

echo ""
echo "Hard disk image formatted successfully."
echo "You can now run './run.sh' and use the 'read /test.txt' command."