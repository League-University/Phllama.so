#!/bin/bash

set -e

echo "Building Phllama.so PHP Extension..."

# Check dependencies
echo "Checking dependencies..."

if ! command -v php-config &> /dev/null; then
    echo "Error: php-config not found. Install PHP development headers."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "Error: cmake not found. Please install cmake."
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo "Error: make not found. Please install build-essential."
    exit 1
fi

# Initialize submodules
echo "Initializing git submodules..."
git submodule update --init --recursive

# Build llama.cpp
echo "Building llama.cpp..."
cd deps/llama.cpp
mkdir -p build
cd build
cmake .. -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF
make -j$(nproc)
cd ../../..

# Build the PHP extension
echo "Building PHP extension..."
make clean
make

echo "Build completed successfully!"
echo ""
echo "To install:"
echo "  sudo make install"
echo "  sudo phpenmod phllama"
echo ""
echo "To test:"
echo "  php test.php"