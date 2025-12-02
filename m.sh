rm -rf build/
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build_scripts/toolchain.cmake
cmake --build build