set(OUTPUT_NAME ${CMAKE_BINARY_DIR}/stage1.bin)

add_custom_command(
    OUTPUT ${OUTPUT_NAME}
    COMMAND ${NASM_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/boot.asm -f bin -o ${OUTPUT_NAME}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/boot.asm
    COMMENT "Assembling stage1.bin"
)

# Create a target to represent the output file
add_custom_target(stage1.bin DEPENDS ${OUTPUT_NAME})