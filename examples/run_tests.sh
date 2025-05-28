#!/bin/bash

# RangeLua Test Runner
# Runs all test scripts in the examples directory progressively

set -e  # Exit on any error

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
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -c, --compare           Compare output with standard Lua 5.5"
    echo "  -v, --verbose           Show detailed output"
    echo "  -s, --stop-on-fail      Stop on first failure"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                      # Run normal tests"
    echo "  $0 -c                   # Run comparison tests"
    echo "  $0 -c -v                # Run comparison tests with verbose output"
}

# Default values
COMPARE=false
VERBOSE=false
STOP_ON_FAIL=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--compare)
            COMPARE=true
            shift
            ;;
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
            echo "Unknown argument: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a single test
run_test() {
    local category=$1
    local test_file=$2
    local full_path="examples/${category}/${test_file}"

    echo -e "${BLUE}Running: ${category}/${test_file}${NC}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ "$COMPARE" = true ]; then
        # Run comparison test
        local rangelua_output=$(mktemp)
        local rangelua_raw_output=$(mktemp)
        local standard_lua_output=$(mktemp)

        # Cleanup function
        cleanup() {
            rm -f "$rangelua_output" "$rangelua_raw_output" "$standard_lua_output"
        }
        trap cleanup EXIT

        # Run RangeLua and capture output
        if ! xmake run rangelua --log-level=off "$full_path" > "$rangelua_raw_output" 2>&1; then
            echo -e "${RED}✗ FAILED - RangeLua execution failed${NC}"
            if [ "$VERBOSE" = true ]; then
                echo -e "${RED}RangeLua output:${NC}"
                cat "$rangelua_raw_output" | sed 's/^/  /'
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi

        # Filter out log lines (lines starting with [timestamp]) to get clean output
        grep -v '^\[20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]' "$rangelua_raw_output" > "$rangelua_output"

        # Run standard Lua and capture output
        if ! "$STANDARD_LUA_PATH" "$full_path" > "$standard_lua_output" 2>&1; then
            echo -e "${RED}✗ FAILED - Standard Lua execution failed${NC}"
            if [ "$VERBOSE" = true ]; then
                echo -e "${RED}Standard Lua output:${NC}"
                cat "$standard_lua_output" | sed 's/^/  /'
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi

        # Compare outputs
        if diff -u "$standard_lua_output" "$rangelua_output" > /dev/null; then
            echo -e "${GREEN}✓ PASSED${NC}"

            if [ "$VERBOSE" = true ]; then
                echo -e "${BLUE}Output:${NC}"
                cat "$standard_lua_output" | sed 's/^/  /'
            fi

            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            echo -e "${RED}✗ FAILED - Outputs differ${NC}"
            if [ "$VERBOSE" = true ]; then
                echo -e "${BLUE}Differences (- Standard Lua, + RangeLua):${NC}"
                diff -u "$standard_lua_output" "$rangelua_output" | sed 's/^/  /' || true
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    else
        # Run normal test
        if xmake build > /dev/null 2>&1 && xmake run rangelua --log-level=off "$full_path" > /dev/null 2>&1; then
            echo -e "${GREEN}✓ PASSED${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            echo -e "${RED}✗ FAILED${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    fi
}

# Function to run tests in a category
run_category() {
    local category=$1
    echo -e "\n${YELLOW}=== Testing Category: $category ===${NC}"

    if [ ! -d "examples/$category" ]; then
        echo -e "${RED}Category directory not found: examples/$category${NC}"
        return 1
    fi

    # Find all .lua files in the category and run them
    for test_file in examples/$category/*.lua; do
        if [ -f "$test_file" ]; then
            local filename=$(basename "$test_file")
            if ! run_test "$category" "$filename"; then
                if [ "$STOP_ON_FAIL" = true ]; then
                    echo -e "\n${RED}Stopping on first failure as requested${NC}"
                    return 1
                fi
            fi
        fi
    done
}

# Main execution
echo -e "${BLUE}RangeLua Progressive Test Suite${NC}"
echo -e "${BLUE}==============================${NC}"

if [ "$COMPARE" = true ]; then
    echo -e "${BLUE}Mode: Comparison with standard Lua 5.5${NC}"

    # Check if standard Lua is available
    if [ ! -f "$STANDARD_LUA_PATH" ]; then
        echo -e "${RED}Error: Standard Lua not found at $STANDARD_LUA_PATH${NC}"
        echo -e "${RED}Please ensure Lua 5.5 is built at the expected location${NC}"
        exit 1
    fi
else
    echo -e "${BLUE}Mode: Development testing${NC}"
fi

# Ensure we're in the right directory
if [ ! -f "xmake.lua" ]; then
    echo -e "${RED}Error: Must be run from RangeLua root directory${NC}"
    exit 1
fi

# Build first
echo -e "\n${YELLOW}Building RangeLua...${NC}"
if ! xmake build; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
echo -e "${GREEN}Build successful${NC}"

# Run tests progressively
categories=("advance" "stdlib")

for category in "${categories[@]}"; do
    if ! run_category "$category"; then
        if [ "$STOP_ON_FAIL" = true ]; then
            break
        fi
    fi
done

# Summary
echo -e "\n${YELLOW}=== Test Summary ===${NC}"
echo -e "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    if [ "$COMPARE" = true ]; then
        echo -e "\n${GREEN}All tests passed! RangeLua output matches standard Lua 5.5! 🎉${NC}"
    else
        echo -e "\n${GREEN}All tests passed! 🎉${NC}"
    fi
    exit 0
else
    if [ "$COMPARE" = true ]; then
        echo -e "\n${RED}Some tests failed. RangeLua output differs from standard Lua 5.5.${NC}"
    else
        echo -e "\n${RED}Some tests failed. Check the output above for details.${NC}"
    fi
    exit 1
fi
