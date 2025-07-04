#!/bin/bash

set -e

echo "Building Phllama.so PHP Extension with Ollama's llama.cpp..."

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

# Check PHP-CPP submodule
echo "Checking PHP-CPP submodule..."
if [ ! -d "deps/php-cpp" ]; then
    echo "Error: PHP-CPP submodule not found. Run 'git submodule update --init --recursive'"
    exit 1
fi

if [ ! -f "deps/php-cpp/CMakeLists.txt" ]; then
    echo "Error: PHP-CPP submodule appears incomplete. Try 'git submodule update --init --recursive'"
    exit 1
fi

# Build the PHP extension (now includes ollama's llama.cpp)
echo "Building PHP extension with ollama's llama.cpp..."
make clean
make

echo "Build completed successfully!"
echo ""
echo "Architecture: Self-contained build with local dependencies"
echo "- Using ollama's patched llama.cpp for enhanced stability"
echo "- PHP-CPP built locally from submodule (no system dependency)"
echo "- Memory management fixes"
echo "- Enhanced CUDA operations" 
echo "- Production-tested patches"
echo ""
echo "To install:"
echo "  sudo make install"
echo "  sudo phpenmod phllama"
echo ""
echo "To test:"
echo "  php test.php"