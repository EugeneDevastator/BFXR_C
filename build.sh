#!/bin/bash
# Build script for bfxr - all dependencies handled by CMake
set -e
cmake -B build -DCMAKE_BUILD_TYPE=${1:-Release} && cmake --build build -- -j$(nproc 2>/dev/null || echo 4)
