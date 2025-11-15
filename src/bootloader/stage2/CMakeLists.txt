# Enable the NASM assembler language for this subdirectory
enable_language(ASM_NASM)

# Find all C and Assembly source files in this directory
file(GLOB C_SOURCES "*.c")
file(GLOB ASM_SOURCES "*.asm")

# Define the executable target
add_executable(stage2.bin
    ${C_SOURCES}
    ${ASM_SOURCES}
)

# Apply compile flags to C and ASM sources separately
set_source_files_properties(${C_SOURCES} PROPERTIES COMPILE_FLAGS "-ffreestanding -nostdlib")

# For NASM, we set the object format property
set_source_files_properties(${ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-f elf32")

# Link with the linker script and required libraries
target_link_options(stage2.bin PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    -nostdlib
)
target_link_libraries(stage2.bin PRIVATE gcc)