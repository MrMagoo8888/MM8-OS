# Enable the NASM assembler language for this subdirectory
enable_language(ASM_NASM)

# --- Generated Sources ---
# Rule to generate the ISR stubs before compiling
set(ISR_GEN_C ${CMAKE_CURRENT_BINARY_DIR}/arch/i686/isrs_gen.c)
set(ISR_GEN_INC ${CMAKE_CURRENT_BINARY_DIR}/arch/i686/isrs_gen.inc)

add_custom_command(
    OUTPUT ${ISR_GEN_C} ${ISR_GEN_INC}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/arch/i686
    COMMAND ${CMAKE_SOURCE_DIR}/build_scripts/generate_isrs.sh ${ISR_GEN_C} ${ISR_GEN_INC}
    COMMENT "Generating ISR stubs"
)

# --- Source Files ---
# Automatically find all source files recursively
file(GLOB_RECURSE KERNEL_C_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.c"
)
file(GLOB_RECURSE KERNEL_ASM_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.asm"
)

# Define the kernel executable, including the generated source file
add_executable(kernel.bin
    ${KERNEL_C_SOURCES}
    ${KERNEL_ASM_SOURCES}
    ${ISR_GEN_C})

# --- Target Properties ---
# Add include directories, including the one for our generated header
target_include_directories(kernel.bin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/i686
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cjson
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Apply compile flags to C and ASM sources separately
set_source_files_properties(${KERNEL_C_SOURCES} ${ISR_GEN_C} PROPERTIES COMPILE_FLAGS "-ffreestanding -nostdlib")
set_source_files_properties(${KERNEL_ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-f elf32")
target_link_options(kernel.bin PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib)
target_link_libraries(kernel.bin PRIVATE gcc)