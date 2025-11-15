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

set(FLOPPY_IMG  ${CMAKE_BINARY_DIR}/main_floppy.img)
set(HDD_IMG     ${CMAKE_BINARY_DIR}/main_hdd.img)

# --- Create an empty HDD image file ---
# This command runs as part of the default build to ensure the file exists.
add_custom_command(OUTPUT ${HDD_IMG}
    COMMAND dd if=/dev/zero of=${HDD_IMG} bs=1M count=32
    COMMENT "Creating empty 32MB HDD image")

add_custom_target(floppy_image ALL
    # This target depends on the bootloader and kernel binaries
    DEPENDS stage1.bin stage2.bin kernel.bin ${HDD_IMG}

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

# --- Custom target to format the HDD ---
# This target is NOT part of the default build and must be run manually.
# It requires sudo because it uses losetup and mount.
add_custom_target(format_hdd
    COMMAND sudo ${CMAKE_SOURCE_DIR}/format_hdd.sh
    COMMENT "Formatting HDD image (requires sudo). Run with: cmake --build build --target format_hdd"
)