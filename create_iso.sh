#!/bin/bash

# This script creates a bootable ISO image from the floppy and HDD images.
# It uses ISOLINUX and MEMDISK to create a "Hybrid ISO" that can be
# burned to CD/DVD OR written directly to a USB stick.

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

# Check for isohybrid (required for USB booting)
if ! command -v isohybrid >/dev/null 2>&1; then
    echo "Error: 'isohybrid' not found. This is required for USB booting."
    echo "Please install 'syslinux-utils' (Debian/Ubuntu) or 'syslinux' (Fedora/Arch)."
    exit 1
fi

# Helper to find system files
find_file() {
    local name=$1
    shift
    for path in "$@"; do
        if [ -f "$path" ]; then echo "$path"; return 0; fi
    done
    return 1
}

# Locate Syslinux/ISOLINUX files (Paths vary by distro)
ISOLINUX_BIN=$(find_file "isolinux.bin" \
    "/usr/lib/ISOLINUX/isolinux.bin" \
    "/usr/lib/syslinux/isolinux.bin" \
    "/usr/lib/syslinux/bios/isolinux.bin" \
    "/usr/share/syslinux/isolinux.bin")

MEMDISK=$(find_file "memdisk" \
    "/usr/lib/syslinux/memdisk" \
    "/usr/lib/syslinux/bios/memdisk" \
    "/usr/share/syslinux/memdisk")

LDLINUX=$(find_file "ldlinux.c32" \
    "/usr/lib/syslinux/modules/bios/ldlinux.c32" \
    "/usr/lib/syslinux/ldlinux.c32" \
    "/usr/share/syslinux/ldlinux.c32")

LIBCOM32=$(find_file "libcom32.c32" \
    "/usr/lib/syslinux/modules/bios/libcom32.c32" \
    "/usr/lib/syslinux/libcom32.c32" \
    "/usr/share/syslinux/libcom32.c32")

LIBUTIL=$(find_file "libutil.c32" \
    "/usr/lib/syslinux/modules/bios/libutil.c32" \
    "/usr/lib/syslinux/libutil.c32" \
    "/usr/share/syslinux/libutil.c32")

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

# Ensure floppy image is exactly 1.44MB (1474560 bytes) for Memdisk
# This prevents Memdisk from treating it as a hard disk if it's too small.
truncate -s 1474560 "$ISO_DIR/$(basename "$FLOPPY_IMG")"

# Build ISO
if [ -n "$ISOLINUX_BIN" ] && [ -n "$MEMDISK" ]; then
    echo "Building Hybrid ISO using ISOLINUX..."
    mkdir -p "$ISO_DIR/isolinux"
    
    cp "$ISOLINUX_BIN" "$ISO_DIR/isolinux/"
    cp "$MEMDISK" "$ISO_DIR/isolinux/"
    # ldlinux.c32 is required for Syslinux 6+, but not 4. Copy if found.
    if [ -n "$LDLINUX" ]; then cp "$LDLINUX" "$ISO_DIR/isolinux/"; fi
    if [ -n "$LIBCOM32" ]; then cp "$LIBCOM32" "$ISO_DIR/isolinux/"; fi
    if [ -n "$LIBUTIL" ]; then cp "$LIBUTIL" "$ISO_DIR/isolinux/"; fi

    # Create ISOLINUX config
    cat > "$ISO_DIR/isolinux/isolinux.cfg" <<EOF
PROMPT 0
TIMEOUT 30
DEFAULT mm8os
LABEL mm8os
    SAY Booting MM8-OS...
    KERNEL memdisk
    APPEND initrd=/$(basename "$FLOPPY_IMG") floppy
EOF

    # Create ISO with ISOLINUX bootloader
    $MKISOFS -J -R -l \
        -b isolinux/isolinux.bin \
        -c isolinux/boot.cat \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        -o "$OUTPUT_ISO" \
        "$ISO_DIR"

    # Apply isohybrid to make it USB-bootable
    echo "Applying isohybrid..."
    isohybrid "$OUTPUT_ISO"
else
    echo "Error: ISOLINUX files (isolinux.bin, memdisk) not found."
    echo "Please install 'syslinux', 'isolinux', and 'syslinux-common' packages."
    exit 1
fi

echo "Success! ISO created at $OUTPUT_ISO"
echo "To boot from USB: sudo dd if=$OUTPUT_ISO of=/dev/sdX bs=4M status=progress"