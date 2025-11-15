# Set the target system name
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)

# This is the crucial part. For freestanding projects, we can't link a test
# executable because there's no C library. By setting this, CMake will
# create a static library for its compiler checks, which avoids the link step.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Specify the cross-compiler paths
# Assumes the toolchain is in toolchain/i686-elf relative to the project root
set(TOOLCHAIN_PREFIX ${CMAKE_SOURCE_DIR}/toolchain/i686-elf)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}/bin/i686-elf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}/bin/i686-elf-g++)

# For ASM_NASM, CMake doesn't have a dedicated compiler variable,
# so we create a custom one and use it later.
find_program(NASM_COMPILER nasm)
if(NOT NASM_COMPILER)
    message(FATAL_ERROR "nasm not found! Please install it.")
endif()

# Manually define the rule for compiling NASM objects. This is often necessary
# for custom toolchains.
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

# Set the find root path to our toolchain. This helps find libraries and headers.
set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)