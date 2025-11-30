set -e # Exit immediately if a command exits with a non-zero status.
rm -rf ./build/
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
cmake --build build
