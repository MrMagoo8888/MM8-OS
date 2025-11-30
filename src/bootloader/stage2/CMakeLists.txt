# Enable the NASM assembler language for this subdirectory
enable_language(ASM_NASM)

# Find all C and Assembly source files in this directory
file(GLOB C_SOURCES "*.c")
file(GLOB ASM_SOURCES "*.asm")

# Define the executable target as an ELF file
add_executable(stage2.elf
    ${C_SOURCES}
    ${ASM_SOURCES}
)

# Link with the linker script and required libraries
target_link_options(stage2.elf PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    -nostdlib # Don't link the C standard library
    -Wl,-Map=${CMAKE_BINARY_DIR}/stage2.map
)

# Explicitly link libgcc
target_link_libraries(stage2.elf PRIVATE gcc)

# Create a flat binary from the ELF file and copy it and the map file
add_custom_command(
    TARGET stage2.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stage2.elf> ${CMAKE_BINARY_DIR}/stage2.bin
    COMMENT "Generating stage2.bin"
)