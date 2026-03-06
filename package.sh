#!/bin/bash

# This script combines the boot floppy and a data partition into a single
# bootable hard disk image. It makes the `format_hdd.sh` script redundant
# for the main boot workflow.

set -e

# --- Configuration ---
FLOPPY_IMAGE="build/main_floppy.img"
HDD_IMAGE="build/packaged.img"
DATA_PART_IMG="build/data_part.img"
MOUNT_POINT="/mnt/mm8os_data"
DATA_SIZE_MB=63 # Size of the data partition

# --- Sudo & Prerequisite Check ---
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (use sudo)"
  exit 1
fi

if [ ! -f "$FLOPPY_IMAGE" ]; then
    echo "Error: Floppy image $FLOPPY_IMAGE not found. Please build it first."
    exit 1
fi

FLOPPY_SIZE_BYTES=$(stat -c%s "$FLOPPY_IMAGE")
FLOPPY_SIZE_MB=$(echo "scale=2; $FLOPPY_SIZE_BYTES / (1024*1024)" | bc)
TOTAL_SIZE_MB=$((DATA_SIZE_MB + 2)) # Add a bit more for the floppy

echo "Floppy image size: $FLOPPY_SIZE_BYTES bytes (~$FLOPPY_SIZE_MB MB)"
echo "Data partition size: $DATA_SIZE_MB MB"
echo "Total image size will be approx $TOTAL_SIZE_MB MB"

# --- Robust Cleanup ---
umount "$MOUNT_POINT" 2>/dev/null || true
if losetup -j "$DATA_PART_IMG" | grep -q "$DATA_PART_IMG"; then
    losetup -d $(losetup -j "$DATA_PART_IMG" | cut -d: -f1) 2>/dev/null || true
fi

# --- 1. Create a temporary data-only filesystem image ---
echo "Creating data partition image: $DATA_PART_IMG"
rm -f "$DATA_PART_IMG"
dd if=/dev/zero of="$DATA_PART_IMG" bs=1M count=$DATA_SIZE_MB

echo "Formatting data partition as FAT32..."
mkfs.fat -F 32 "$DATA_PART_IMG"

# --- 2. Mount and copy files to the data partition ---
echo "Mounting data partition to copy files..."
mkdir -p "$MOUNT_POINT"
mount "$DATA_PART_IMG" "$MOUNT_POINT"

echo "Copying files..."
cp "test.txt" "$MOUNT_POINT/test.txt"
cp "blank.bmp" "$MOUNT_POINT/blank.bmp"
cp "bblack.bmp" "$MOUNT_POINT/bblack.bmp"

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

echo "Unmounting data partition..."
umount "$MOUNT_POINT"

# --- 3. Combine floppy and data partition into a single HDD image ---
echo "Combining $FLOPPY_IMAGE and $DATA_PART_IMG into $HDD_IMAGE..."
rm -f "$HDD_IMAGE"
cat "$FLOPPY_IMAGE" "$DATA_PART_IMG" > "$HDD_IMAGE"

# --- 4. Clean up temporary files and fix permissions ---
echo "Cleaning up..."
rm "$DATA_PART_IMG"

if [ ! -z "$SUDO_USER" ]; then
    echo "Fixing permissions for $HDD_IMAGE..."
    chown "$SUDO_USER" "$HDD_IMAGE"
fi

echo -e "\nSuccessfully created bootable image at $HDD_IMAGE"
echo "You can now update run.sh to use it."
echo -e "\nIMPORTANT: Your kernel must access the filesystem at an offset."
echo "Offset in bytes: $FLOPPY_SIZE_BYTES"
echo "Offset in sectors (512 bytes): $(($FLOPPY_SIZE_BYTES / 512))"