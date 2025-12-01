# Enable NASM assembler if assembly files are present in stage2
enable_language(ASM_NASM)

# Find all C and Assembly source files
file(GLOB_RECURSE C_SOURCES "*.c")
file(GLOB_RECURSE ASM_SOURCES "*.asm")

# Define the stage2 executable target as an ELF file
add_executable(stage2.elf
    ${C_SOURCES}
    ${ASM_SOURCES}
    # Add the linker script to the sources so CMake knows about it and rebuilds if it changes
    ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

# Add include directories for stage2 if needed
# target_include_directories(stage2.elf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link with required libraries and generate a map file
target_link_options(stage2.elf PRIVATE
    -nostdlib # Don't link the C standard library
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld # Use the custom linker script
    -Wl,-Map=${CMAKE_BINARY_DIR}/stage2.map # Generate a map file
    -Wl,--build-id=none # Prevent linker from adding a build ID, often useful for bare-metal
)

# Explicitly link libgcc, similar to kernel.elf
target_link_libraries(stage2.elf PRIVATE gcc)

# Create a flat binary from the ELF file and copy it to the build directory
add_custom_command(
    TARGET stage2.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stage2.elf> ${CMAKE_BINARY_DIR}/stage2.bin
    COMMENT "Generating stage2.bin"
)