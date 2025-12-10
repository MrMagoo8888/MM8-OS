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

echo "Ensuring build directory and HDD image exist..."
mkdir -p "$(dirname "$HDD_IMAGE")"
if [ ! -f "$HDD_IMAGE" ]; then
    echo "Creating 100MB disk image at $HDD_IMAGE..."
    # Create a 100MB file for the hard disk image
    dd if=/dev/zero of="$HDD_IMAGE" bs=1M count=100
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

echo "Creating FAT32 filesystem on $LOOP_DEV..."
# We use --mbr=y to write a Master Boot Record (MBR) to the disk image.
# This creates a partition table and the 0xAA55 boot signature, which is required for the disk to be recognized as a valid, formatted drive.
mkfs.fat -F 32 --mbr=y "$LOOP_DEV"

echo "Creating mount point $MOUNT_POINT..."
mkdir -p "$MOUNT_POINT"

echo "Mounting $LOOP_DEV to $MOUNT_POINT..."
mount "$LOOP_DEV" "$MOUNT_POINT"

echo "Copying test files to the disk image..."
cp "$TEST_FILE" "$MOUNT_POINT/test.txt"

cat > "$MOUNT_POINT/test.json" << EOL
{
    "message": "Hello from JSON!",
    "kernel": "MM8-OS",
    "year": 2025,
    "is_awesome": true,
    "features": [
        "FAT32", "malloc", "cJSON"
    ]
}
EOL

echo "Unmounting and detaching loopback device..."
umount "$MOUNT_POINT"
losetup -d "$LOOP_DEV"

echo "Changing ownership of disk image to the original user..."
# Change ownership back to the user who ran sudo, so qemu can access it.
if [ -n "$SUDO_USER" ] && [ -n "$SUDO_GID" ]; then
    chown "$SUDO_USER:$SUDO_GID" "$HDD_IMAGE"
fi

echo ""
echo "Hard disk image formatted successfully."
echo "You can now run './run.sh' and use the 'read /test.txt' command."