# The first line is commented out as it causes a "write lock" error.
# We will use the second line, which boots from a floppy and uses a separate hard disk image.
qemu-system-i386 -boot order=a -drive file=build/main_floppy.img,format=raw,if=floppy -drive file=build/main_hdd.img,format=raw,if=ide,index=0,media=disk