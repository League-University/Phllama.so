#!/bin/bash

# Phllama.so Installation Validator
# Validates that the extension is properly installed and configured

set -e

echo "🔍 Phllama.so Installation Validator"
echo "═══════════════════════════════════════"
echo

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check functions
check_php() {
    echo -n "Checking PHP installation... "
    if command -v php &> /dev/null; then
        PHP_VERSION=$(php -v | head -n1 | cut -d' ' -f2)
        echo -e "${GREEN}✓${NC} PHP $PHP_VERSION found"
    else
        echo -e "${RED}✗${NC} PHP not found"
        exit 1
    fi
}

check_extension_file() {
    echo -n "Checking extension file... "
    EXT_DIR=$(php-config --extension-dir)
    if [ -f "$EXT_DIR/phllama.so" ]; then
        echo -e "${GREEN}✓${NC} phllama.so found in $EXT_DIR"
    else
        echo -e "${RED}✗${NC} phllama.so not found in $EXT_DIR"
        echo "  Run: sudo make install"
        exit 1
    fi
}

check_extension_loaded() {
    echo -n "Checking if extension is loaded... "
    if php -m | grep -q phllama; then
        echo -e "${GREEN}✓${NC} phllama extension is loaded"
    else
        echo -e "${RED}✗${NC} phllama extension not loaded"
        echo "  Run: sudo phpenmod phllama"
        exit 1
    fi
}

check_basic_functionality() {
    echo -n "Testing basic functionality... "
    if php -r "new Phllama('');" 2>&1 | grep -q "cannot be empty"; then
        echo -e "${GREEN}✓${NC} Extension responds correctly"
    else
        echo -e "${RED}✗${NC} Extension not responding correctly"
        exit 1
    fi
}

check_ollama_optional() {
    echo -n "Checking ollama (optional)... "
    if command -v ollama &> /dev/null; then
        OLLAMA_VERSION=$(ollama --version 2>/dev/null || echo "unknown")
        echo -e "${GREEN}✓${NC} Ollama found: $OLLAMA_VERSION"
    else
        echo -e "${YELLOW}⚠${NC} Ollama not found (optional for direct file loading)"
    fi
}

check_dependencies() {
    echo -n "Checking PHP-CPP... "
    if [ -f "/usr/local/lib/libphpcpp.so" ] || [ -f "/usr/lib/libphpcpp.so" ]; then
        echo -e "${GREEN}✓${NC} PHP-CPP library found"
    else
        echo -e "${YELLOW}⚠${NC} PHP-CPP library may not be installed"
    fi
}

# Run all checks
echo "Running installation validation checks..."
echo

check_php
check_dependencies
check_extension_file
check_extension_loaded
check_basic_functionality
check_ollama_optional

echo
echo -e "${GREEN}🎉 All checks passed!${NC}"
echo
echo "Next steps:"
echo "1. Run 'php test.php' for comprehensive testing"
echo "2. Try the examples in README.md"
echo "3. Check RELEASE_NOTES.md for alpha limitations"
echo
echo "For help: https://github.com/your-org/phllama.so"