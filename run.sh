# This script now boots from a single, combined hard disk image.
# Use 'sudo ./package.sh' to create 'build/packaged.img' before running this.

qemu-system-i386 -boot order=c -drive file=build/packaged.img,format=raw,if=ide,index=0,media=disk -display sdl -d int,cpu_reset,guest_errors -D qemu.log -m 4096 -no-reboot