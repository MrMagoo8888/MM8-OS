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
    DEPENDS stage1.bin stage2.bin kernel.bin

    # The sequence of commands to build the image
    COMMAND ${CMAKE_COMMAND} -E echo "--> Creating Floppy Image: ${FLOPPY_IMG}"
    COMMAND dd if=/dev/zero of=${FLOPPY_IMG} bs=512 count=2880
    COMMAND mkfs.fat -F 12 -n "MM8OS" ${FLOPPY_IMG}
    COMMAND dd if=${CMAKE_BINARY_DIR}/stage1.bin of=${FLOPPY_IMG} conv=notrunc

    # Use mcopy to add files to the FAT filesystem
    COMMAND mcopy -i ${FLOPPY_IMG} $<TARGET_FILE:stage2.bin> "::stage2.bin"
    COMMAND mcopy -i ${FLOPPY_IMG} $<TARGET_FILE:kernel.bin> "::kernel.bin"
    COMMAND mcopy -i ${FLOPPY_IMG} ${CMAKE_SOURCE_DIR}/test.txt "::test.txt"
    COMMAND mcopy -i ${FLOPPY_IMG} ${CMAKE_SOURCE_DIR}/test.json "::test.json"
    COMMAND mmd -i ${FLOPPY_IMG} "::mydir"
    COMMAND mcopy -i ${FLOPPY_IMG} ${CMAKE_SOURCE_DIR}/test.txt "::mydir/test.txt"

    # Use VERBATIM to handle shell redirection and special characters correctly
    VERBATIM
    COMMENT "Building main_floppy.img"
)