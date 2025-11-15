# Set the minimum required version of CMake and define the project
cmake_minimum_required(VERSION 3.15)
project(MM8-OS LANGUAGES C CXX ASM)

# --- Add subdirectories for each component ---
# Each of these directories will contain its own CMakeLists.txt
add_subdirectory(src/bootloader/stage1)
add_subdirectory(src/bootloader/stage2)
add_subdirectory(src/kernel)

# --- Custom Target for Floppy Image Creation ---
# This replaces the floppy_image target from your Makefile

set(FLOPPY_IMG ${CMAKE_BINARY_DIR}/main_floppy.img)

add_custom_target(floppy_image ALL
    # This target depends on the bootloader and kernel binaries
    DEPENDS build_stage1 stage2.bin kernel.bin

    # Chain all image creation steps into a single command to ensure execution order.
    COMMAND sh -c " \
        echo '--> Creating Floppy Image: ${FLOPPY_IMG}' && \
        dd if=/dev/zero of=${FLOPPY_IMG} bs=512 count=2880 && \
        mkfs.fat -F 12 -n 'MM8OS' -r 224 ${FLOPPY_IMG} >/dev/null 2>&1 && \
        dd if=${CMAKE_BINARY_DIR}/stage1.bin of=${FLOPPY_IMG} conv=notrunc >/dev/null 2>&1 && \
        mcopy -i ${FLOPPY_IMG} ${CMAKE_BINARY_DIR}/src/bootloader/stage2/stage2.bin '::STAGE2.BIN' >/dev/null 2>&1 && \
        mcopy -i ${FLOPPY_IMG} ${CMAKE_BINARY_DIR}/src/kernel/kernel.bin '::KERNEL.BIN' >/dev/null 2>&1"

    # Use VERBATIM to handle shell redirection and special characters correctly
    VERBATIM
    COMMENT "Building main_floppy.img"
)