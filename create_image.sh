#!/bin/bash

# This script creates a single, bootable, partitioned hard disk image.

set -e

# --- Configuration ---
IMAGE_FILE="build/os.img"
IMAGE_SIZE_MB=64
MOUNT_POINT="/mnt/mm8os"

# --- Sudo & Prerequisite Check ---
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (use sudo)"
  exit 1
fi

# --- Robust Cleanup ---
umount "$MOUNT_POINT" 2>/dev/null || true
if losetup -j "$IMAGE_FILE" | grep -q "$IMAGE_FILE"; then
    losetup -d $(losetup -j "$IMAGE_FILE" | cut -d: -f1) 2>/dev/null || true
fi

# --- Create the disk image ---
echo "Creating disk image: $IMAGE_FILE..."
rm -f "$IMAGE_FILE"
dd if=/dev/zero of="$IMAGE_FILE" bs=1M count=$IMAGE_SIZE_MB

# --- Create partition table using parted ---
echo "Creating partition table..."
# We create a standard msdos partition table.
# We create a primary partition starting at 1MiB, leaving space for the bootloader.
parted "$IMAGE_FILE" --script -- mklabel msdos
parted "$IMAGE_FILE" --script -- mkpart primary fat32 1MiB 100%
parted "$IMAGE_FILE" --script -- set 1 boot on

# --- Install Bootloader ---
echo "Installing bootloader to MBR..."
# The mkfs.fat command creates a generic MBR. We need to overwrite its boot code
# with our own stage 1 bootloader, without touching the partition table (bytes 446-510).
dd if=build/main_floppy.img of="$IMAGE_FILE" bs=446 count=1 conv=notrunc

# Our stage 1 bootloader loads stage 2 from the sectors immediately after the MBR.
# We'll copy 31 sectors of stage 2 into that gap.
dd if=build/main_floppy.img of="$IMAGE_FILE" bs=512 skip=1 seek=1 count=31 conv=notrunc

# --- Mount and copy files ---
echo "Setting up loopback device..."
LOOP_DEV=$(losetup -fP --show "$IMAGE_FILE")
if [ -z "$LOOP_DEV" ]; then
    echo "Failed to set up loopback device."
    exit 1
fi

# --- Format the partition ---
echo "Formatting partition ${LOOP_DEV}p1..."
mkfs.fat -F 32 "${LOOP_DEV}p1"

echo "Mounting partition to copy files..."
mkdir -p "$MOUNT_POINT"
mount "${LOOP_DEV}p1" "$MOUNT_POINT"

echo "Copying files to the disk image..."
cp build/kernel.bin "$MOUNT_POINT/kernel.bin"
cp test.txt "$MOUNT_POINT/test.txt"
cp test.json "$MOUNT_POINT/test.json"
cp blank.bmp "$MOUNT_POINT/blank.bmp"
cp bblack.bmp "$MOUNT_POINT/bblack.bmp"

echo "Unmounting and detaching loopback device..."
umount "$MOUNT_POINT"
losetup -d "$LOOP_DEV"

# --- Fix permissions ---
if [ ! -z "$SUDO_USER" ]; then
    echo "Fixing permissions for $IMAGE_FILE..."
    chown "$SUDO_USER" "$IMAGE_FILE"
fi

echo ""
echo "Bootable hard disk image created successfully at $IMAGE_FILE."
echo "Update run.sh to use 'build/os.img' and then run your OS."