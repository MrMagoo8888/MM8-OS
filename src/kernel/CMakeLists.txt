# Enable NASM assembler
enable_language(ASM_NASM) # This is correct, no change needed, but good to confirm.

# Find all C and Assembly source files
file(GLOB_RECURSE C_SOURCES "*.c")
file(GLOB_RECURSE ASM_SOURCES "*.asm")

# Define the kernel executable target as an ELF file
add_executable(kernel.elf
    ${C_SOURCES}
    ${ASM_SOURCES}
)

# Add include directories for the kernel
# This allows files like calc.c to find headers like stdio.h
target_include_directories(kernel.elf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# Link with required libraries and generate a map file
target_link_options(kernel.elf PRIVATE
    -nostdlib # Don't link the C standard library
    -Wl,-Map=${CMAKE_BINARY_DIR}/kernel.map
)

# Explicitly link libgcc
target_link_libraries(kernel.elf PRIVATE gcc)

# Create a flat binary from the ELF file and copy it and the map file
add_custom_command(
    TARGET kernel.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:kernel.elf> ${CMAKE_BINARY_DIR}/kernel.bin
    COMMENT "Generating kernel.bin"
)