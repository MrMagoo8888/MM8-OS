!! FAT SUBSYSTEM BRANCH LASTEST WORKING BRANCH!!


Hello, I am making this OS for fun and would love help, tips and suggestions!

Prequisits:

Linux-based distro (I have ubuntu, others may or may not work natively)

BASH shell

(apt package manager)


Basic APT packages

Open-Watcom C compiler

GCC

MAKE

NASM

QEMU


Any Text Editor


(there may be others I am missing)

```sh
# Ubuntu, Debian:
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget \
                   nasm mtools python3 python3-pip python3-parted scons dosfstools libguestfs-tools qemu-system-x86

# Fedora:
sudo dnf install gcc gcc-c++ make bison flex gmp-devel libmpc-devel mpfr-devel texinfo wget \
                   nasm mtools python3 python3-pip python3-pyparted python3-scons dosfstools guestfs-tools qemu-system-x86

# Arch & Arch-based:
paru -S gcc make bison flex libgmp-static libmpc mpfr texinfo nasm mtools qemu-system-x86 python3 scons
```#

NOTE: to install all the required packages on Arch, you need an [AUR helper](https://wiki.archlinux.org/title/AUR_helpers).

```
Thanks to Nanobyte for their OS Development series that has greatly helped with this feat.
(https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN)
