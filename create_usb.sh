#!/bin/bash

# This script creates a bootable USB disk image using Syslinux and MEMDISK.
# It requires sudo privileges to mount and format the image.

set -e

BUILD_DIR="build"
USB_IMG="${BUILD_DIR}/mm8os_usb.img"
FLOPPY_IMG="${BUILD_DIR}/main_floppy.img"
MOUNT_POINT="/mnt/mm8os_usb"

# Check for root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (use sudo)"
  exit 1
fi

# Check dependencies
command -v syslinux >/dev/null 2>&1 || { echo "Error: syslinux not found. Please install syslinux and syslinux-utils."; exit 1; }
command -v mkfs.fat >/dev/null 2>&1 || { echo "Error: mkfs.fat not found. Please install dosfstools."; exit 1; }
command -v sfdisk >/dev/null 2>&1 || { echo "Error: sfdisk not found."; exit 1; }

# Locate memdisk and mbr.bin (Paths vary by distro)
MEMDISK_PATH=""
MBR_PATH=""

# Common locations for Debian/Ubuntu/Fedora
POSSIBLE_MEMDISK_PATHS=(
    "/usr/lib/syslinux/memdisk"
    "/usr/lib/syslinux/bios/memdisk"
    "/usr/share/syslinux/memdisk"
)

POSSIBLE_MBR_PATHS=(
    "/usr/lib/syslinux/mbr/mbr.bin"
    "/usr/lib/syslinux/bios/mbr.bin"
    "/usr/lib/syslinux/mbr.bin"
)

for p in "${POSSIBLE_MEMDISK_PATHS[@]}"; do
    if [ -f "$p" ]; then MEMDISK_PATH="$p"; break; fi
done

for p in "${POSSIBLE_MBR_PATHS[@]}"; do
    if [ -f "$p" ]; then MBR_PATH="$p"; break; fi
done

if [ -z "$MEMDISK_PATH" ]; then echo "Error: memdisk binary not found."; exit 1; fi
if [ -z "$MBR_PATH" ]; then echo "Error: mbr.bin not found."; exit 1; fi

echo "Creating empty disk image (128MB)..."
dd if=/dev/zero of="$USB_IMG" bs=1M count=128 status=none

echo "Partitioning image..."
# Create a single bootable FAT32 partition (Type 0x0c)
echo "type=c,bootable" | sfdisk "$USB_IMG" >/dev/null

echo "Setting up loopback device..."
# -P scans for partitions so we get /dev/loopXp1
LOOP_DEV=$(losetup -P -f --show "$USB_IMG")
PART_DEV="${LOOP_DEV}p1"

echo "Formatting partition as FAT32..."
mkfs.fat -F 32 -n "MM8OS_USB" "$PART_DEV" >/dev/null

echo "Installing Syslinux..."
syslinux --install "$PART_DEV"

echo "Writing Master Boot Record (MBR)..."
dd if="$MBR_PATH" of="$LOOP_DEV" conv=notrunc bs=440 count=1 status=none

echo "Mounting and copying files..."
mkdir -p "$MOUNT_POINT"
mount "$PART_DEV" "$MOUNT_POINT"

cp "$MEMDISK_PATH" "$MOUNT_POINT/memdisk"
cp "$FLOPPY_IMG" "$MOUNT_POINT/mm8os.img"

# Create syslinux configuration
cat > "$MOUNT_POINT/syslinux.cfg" <<EOF
DEFAULT mm8os
LABEL mm8os
    SAY Booting MM8-OS via MEMDISK...
    KERNEL memdisk
    INITRD mm8os.img
EOF

echo "Cleaning up..."
umount "$MOUNT_POINT"
losetup -d "$LOOP_DEV"

# Fix permissions so the regular user can run QEMU
if [ ! -z "$SUDO_USER" ]; then
    chown "$SUDO_USER" "$USB_IMG"
fi

echo "Success! USB Image created at $USB_IMG"
echo "To write to a real USB stick: sudo dd if=$USB_IMG of=/dev/sdX bs=4M status=progress"