set(OUTPUT_NAME ${CMAKE_BINARY_DIR}/stage1.bin)

add_custom_command(
    OUTPUT ${OUTPUT_NAME}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o ${OUTPUT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/boot.asm
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/boot.asm
    COMMENT "Assembling stage1.bin"
)

# Create a target to represent the output file
add_custom_target(build_stage1 DEPENDS ${OUTPUT_NAME})