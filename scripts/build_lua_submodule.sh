#!/bin/bash

# Script to build the Lua submodule for validation and reference
# This script builds the official Lua interpreter from the git submodule
# for use in integration tests as a reference implementation

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
LUA_DIR="$PROJECT_ROOT/third_party/lua"

echo "Building Lua submodule for integration testing..."

# Check if the submodule directory exists
if [ ! -d "$LUA_DIR" ]; then
    echo "Error: Lua submodule directory not found at $LUA_DIR"
    echo "Please make sure the git submodule is initialized:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

# Check if Lua source files exist
if [ ! -f "$LUA_DIR/lua.c" ]; then
    echo "Error: Lua source files not found in $LUA_DIR"
    echo "Please make sure the git submodule is properly initialized:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

# Change to the Lua directory
cd "$LUA_DIR"

echo "Building Lua in $LUA_DIR..."

# Clean any previous build
make clean 2>/dev/null || true

# Build Lua
# Use the default makefile which should work on most Linux systems
make

# Check if the build was successful
if [ -f "lua" ] && [ -x "lua" ]; then
    echo "✓ Lua interpreter built successfully: $LUA_DIR/lua"

    # Test the built interpreter
    echo "Testing the built Lua interpreter..."
    ./lua -v

    echo "✓ Lua submodule is ready for integration testing!"
else
    echo "✗ Failed to build Lua interpreter"
    exit 1
fi

echo ""
echo "You can now run integration tests that will use this Lua interpreter for validation."
echo "The tests will automatically detect and use: $LUA_DIR/lua"
echo ""
echo "Note: This Lua interpreter is used for output validation, not for comprehensive"
echo "      compatibility testing, as RangeLua is still in early development."
