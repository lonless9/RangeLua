#!/bin/bash

# RangeLua Comparison Test Runner
# Compares RangeLua output with standard Lua 5.5 output

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
STANDARD_LUA_PATH="$HOME/code/cpp/lua/lua"

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] <test_file_or_directory>"
    echo ""
    echo "Options:"
    echo "  -v, --verbose           Show detailed output including successful comparisons"
    echo "  -s, --stop-on-fail      Stop on first failure"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 examples/stdlib/test_basic.lua"
    echo "  $0 examples/stdlib/"
    echo "  $0 -v examples/advance/comprehensive_basic.lua"
}

# Default values
VERBOSE=false
STOP_ON_FAIL=false
TARGET=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -s|--stop-on-fail)
            STOP_ON_FAIL=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        -*)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
        *)
            TARGET="$1"
            shift
            ;;
    esac
done

# Check if target is provided
if [ -z "$TARGET" ]; then
    echo -e "${RED}Error: No test file or directory specified${NC}"
    show_usage
    exit 1
fi

# Check if target exists
if [ ! -e "$TARGET" ]; then
    echo -e "${RED}Error: Target not found: $TARGET${NC}"
    exit 1
fi

# Check if standard Lua is available
if [ ! -f "$STANDARD_LUA_PATH" ]; then
    echo -e "${RED}Error: Standard Lua not found at $STANDARD_LUA_PATH${NC}"
    echo -e "${RED}Please ensure Lua 5.5 is built at the expected location${NC}"
    exit 1
fi

# Ensure we're in the right directory
if [ ! -f "xmake.lua" ]; then
    echo -e "${RED}Error: Must be run from RangeLua root directory${NC}"
    exit 1
fi

# Build first
echo -e "${YELLOW}Building RangeLua...${NC}"
if ! xmake build > /dev/null 2>&1; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
echo -e "${GREEN}Build successful${NC}"

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to compare single file
compare_file() {
    local test_file="$1"
    local relative_path="${test_file#examples/}"

    echo -e "${BLUE}Testing: $relative_path${NC}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # Create temporary files for outputs
    local rangelua_output=$(mktemp)
    local rangelua_raw_output=$(mktemp)
    local standard_lua_output=$(mktemp)

    # Cleanup function
    cleanup() {
        rm -f "$rangelua_output" "$rangelua_raw_output" "$standard_lua_output"
    }
    trap cleanup EXIT

    # Run RangeLua and capture output
    if ! xmake run rangelua --log-level=off "$test_file" > "$rangelua_raw_output" 2>&1; then
        echo -e "${RED}  ✗ RangeLua execution failed${NC}"
        if [ "$VERBOSE" = true ]; then
            echo -e "${RED}  RangeLua output:${NC}"
            cat "$rangelua_raw_output" | sed 's/^/    /'
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi

    # Filter out log lines (lines starting with [timestamp]) to get clean output
    grep -v '^\[20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]' "$rangelua_raw_output" > "$rangelua_output"

    # Run standard Lua and capture output
    if ! "$STANDARD_LUA_PATH" "$test_file" > "$standard_lua_output" 2>&1; then
        echo -e "${RED}  ✗ Standard Lua execution failed${NC}"
        if [ "$VERBOSE" = true ]; then
            echo -e "${RED}  Standard Lua output:${NC}"
            cat "$standard_lua_output" | sed 's/^/    /'
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi

    # Compare outputs
    if diff -u "$standard_lua_output" "$rangelua_output" > /dev/null; then
        echo -e "${GREEN}  ✓ PASSED${NC}"

        if [ "$VERBOSE" = true ]; then
            echo -e "${BLUE}  Output:${NC}"
            cat "$standard_lua_output" | sed 's/^/    /'
        fi

        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}  ✗ FAILED - Outputs differ${NC}"
        echo -e "${BLUE}  Differences (- Standard Lua, + RangeLua):${NC}"
        diff -u "$standard_lua_output" "$rangelua_output" | sed 's/^/    /' || true

        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Main execution
echo -e "${BLUE}RangeLua vs Standard Lua 5.5 Comparison${NC}"
echo -e "${BLUE}=======================================${NC}"

if [ -f "$TARGET" ]; then
    # Single file
    compare_file "$TARGET"
elif [ -d "$TARGET" ]; then
    # Directory - find all .lua files
    echo -e "\n${YELLOW}Scanning directory: $TARGET${NC}"

    while IFS= read -r -d '' test_file; do
        compare_file "$test_file"

        if [ "$STOP_ON_FAIL" = true ] && [ $FAILED_TESTS -gt 0 ]; then
            echo -e "\n${RED}Stopping on first failure as requested${NC}"
            break
        fi
    done < <(find "$TARGET" -name "*.lua" -type f -print0 | sort -z)
fi

# Summary
echo -e "\n${YELLOW}=== Comparison Summary ===${NC}"
echo -e "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! RangeLua output matches standard Lua 5.5! 🎉${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed. RangeLua output differs from standard Lua 5.5.${NC}"
    exit 1
fi
