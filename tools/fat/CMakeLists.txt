cmake_minimum_required(VERSION 3.15)
project(fat_tool LANGUAGES C)

add_executable(fat fat.c)

install(TARGETS fat DESTINATION ${CMAKE_BINARY_DIR}/bin)