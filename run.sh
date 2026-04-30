# This script now boots from a single, combined hard disk image.
# Use 'sudo ./package.sh' to create 'build/packaged.img' before running this.

#qemu-system-i386 -boot order=c -drive file=build/packaged.img,format=raw,if=ide,index=0,media=disk -d int,cpu_reset,guest_errors -D qemu.log -m 4096 -no-reboot
qemu-system-i386 -boot order=c -drive file=build/packaged.img,format=raw,if=none,id=usb_stick -device pci-ohci,id=ohci -device usb-storage,bus=ohci.0,drive=usb_stick -m 512