# Set the system name to Generic, which is used for embedded systems
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Specify the path to the cross-compiler. Assumes it's in the project directory.
set(TOOLCHAIN_PATH ${CMAKE_SOURCE_DIR}/toolchain/i686-elf)
set(TOOLCHAIN_PREFIX ${TOOLCHAIN_PATH}/bin/i686-elf-)

# Set compilers for C, C++, and ASM
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
set(CMAKE_ASM_NASM_COMPILER nasm)

# Set compiler flags
set(CMAKE_C_FLAGS "-ffreestanding -O2 -Wall -Wextra -m32" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS "-ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -m32" CACHE STRING "C++ compiler flags")
set(CMAKE_C_LINK_FLAGS "-ffreestanding" CACHE STRING "C Linker Flags")
set(CMAKE_ASM_NASM_FLAGS "-f elf32" CACHE STRING "NASM assembler flags")

# IMPORTANT: Specify the C compiler as the linker.
# This ensures that 'i686-elf-gcc' is used to drive the linking process,
# which knows how to correctly link 32-bit object files and find libgcc.
set(CMAKE_C_LINKER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_LINKER ${CMAKE_CXX_COMPILER})

# Don't run the host OS check
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")