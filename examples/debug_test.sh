#!/bin/bash

# RangeLua Debug Test Runner
# Runs individual tests with detailed logging for debugging

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] <test_file>"
    echo ""
    echo "Options:"
    echo "  -m, --module <module>    Enable debug logging for specific module"
    echo "                          (lexer, parser, codegen, optimizer, vm, memory, gc)"
    echo "  -l, --log-level <level>  Set overall log level (trace, debug, info, warn, error, off)"
    echo "                          When used alone, enables all modules at specified level"
    echo "  -a, --all-debug         Enable debug logging for all modules (same as -l debug)"
    echo "  -c, --compare           Compare output with standard Lua 5.5"
    echo "  -v, --verbose           Show detailed output"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 examples/stdlib/test_basic.lua                    # Clean execution (no logs)"
    echo "  $0 -m parser examples/stdlib/test_basic.lua          # Only parser debug logs"
    echo "  $0 -l debug examples/stdlib/test_basic.lua           # All modules debug logs"
    echo "  $0 -a examples/stdlib/test_basic.lua                 # All modules debug logs"
    echo "  $0 -c examples/stdlib/test_basic.lua                 # Compare with standard Lua"
    echo "  $0 -c -v examples/stdlib/test_basic.lua              # Compare with verbose output"
}

# Default values
MODULE=""
LOG_LEVEL=""
VERBOSE=false
ALL_DEBUG=false
COMPARE=false
TEST_FILE=""
STANDARD_LUA_PATH="$HOME/code/cpp/lua/lua"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--module)
            MODULE="$2"
            shift 2
            ;;
        -l|--log-level)
            LOG_LEVEL="$2"
            shift 2
            ;;
        -a|--all-debug)
            ALL_DEBUG=true
            shift
            ;;
        -c|--compare)
            COMPARE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
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
            TEST_FILE="$1"
            shift
            ;;
    esac
done

# Check if test file is provided
if [ -z "$TEST_FILE" ]; then
    echo -e "${RED}Error: No test file specified${NC}"
    show_usage
    exit 1
fi

# Check if test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${RED}Error: Test file not found: $TEST_FILE${NC}"
    exit 1
fi

# Check if standard Lua is available when compare mode is enabled
if [ "$COMPARE" = true ] && [ ! -f "$STANDARD_LUA_PATH" ]; then
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
if ! xmake build; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
echo -e "${GREEN}Build successful${NC}"

# Prepare command arguments
CMD_ARGS=()

# New logging activation strategy:
# 1. If specific module is requested, use explicit module logging
# 2. If ALL_DEBUG or LOG_LEVEL is specified, use the new automatic all-module activation
# 3. If nothing specified, keep clean (no logging)

if [ -n "$MODULE" ]; then
    # Explicit module logging: activate only specified module
    CMD_ARGS+=(--module-log "${MODULE}:debug")
elif [ "$ALL_DEBUG" = true ]; then
    # All-module logging via log level (new strategy)
    CMD_ARGS+=(--log-level "debug")
elif [ -n "$LOG_LEVEL" ]; then
    # All-module logging at specified level (new strategy)
    CMD_ARGS+=(--log-level "$LOG_LEVEL")
fi
# If none of the above, no logging args = clean output

CMD_ARGS+=("$TEST_FILE")

# Show what we're running
echo -e "\n${BLUE}Running test:${NC}"
echo -e "${BLUE}File: $TEST_FILE${NC}"
if [ "$COMPARE" = true ]; then
    echo -e "${BLUE}Mode: Comparison with standard Lua 5.5${NC}"
else
    echo -e "${BLUE}Mode: Debug/Development${NC}"
fi
if [ "$ALL_DEBUG" = true ]; then
    echo -e "${BLUE}Debug: All modules${NC}"
elif [ -n "$MODULE" ]; then
    echo -e "${BLUE}Debug: $MODULE module${NC}"
fi
if [ -n "$LOG_LEVEL" ]; then
    echo -e "${BLUE}Log Level: $LOG_LEVEL${NC}"
fi

# Function to run comparison test
run_comparison_test() {
    echo -e "\n${YELLOW}=== Comparison Test ===${NC}"

    # Create temporary files for outputs
    local rangelua_output=$(mktemp)
    local standard_lua_output=$(mktemp)

    # Cleanup function
    cleanup() {
        rm -f "$rangelua_output" "$standard_lua_output" "$rangelua_raw_output"
    }
    trap cleanup EXIT

    echo -e "${BLUE}Running RangeLua...${NC}"
    if [ "$VERBOSE" = true ]; then
        echo -e "${BLUE}Command: xmake run rangelua ${CMD_ARGS[*]}${NC}"
    fi

    # Run RangeLua and capture output (force log level to off for comparison)
    local rangelua_cmd_args=("--log-level=off" "$TEST_FILE")
    local rangelua_raw_output=$(mktemp)
    if ! xmake run rangelua "${rangelua_cmd_args[@]}" > "$rangelua_raw_output" 2>&1; then
        echo -e "${RED}✗ RangeLua execution failed${NC}"
        echo -e "${RED}RangeLua output:${NC}"
        cat "$rangelua_raw_output"
        return 1
    fi

    # Filter out log lines (lines starting with [timestamp]) to get clean output
    grep -v '^\[20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]' "$rangelua_raw_output" > "$rangelua_output"

    echo -e "${BLUE}Running standard Lua 5.5...${NC}"
    if [ "$VERBOSE" = true ]; then
        echo -e "${BLUE}Command: $STANDARD_LUA_PATH $TEST_FILE${NC}"
    fi

    # Run standard Lua and capture output
    if ! "$STANDARD_LUA_PATH" "$TEST_FILE" > "$standard_lua_output" 2>&1; then
        echo -e "${RED}✗ Standard Lua execution failed${NC}"
        echo -e "${RED}Standard Lua output:${NC}"
        cat "$standard_lua_output"
        return 1
    fi

    echo -e "\n${YELLOW}=== Output Comparison ===${NC}"

    # Compare outputs
    if diff -u "$standard_lua_output" "$rangelua_output" > /dev/null; then
        echo -e "${GREEN}✓ Outputs match perfectly!${NC}"

        if [ "$VERBOSE" = true ]; then
            echo -e "\n${BLUE}Standard Lua output:${NC}"
            cat "$standard_lua_output"
        fi
        return 0
    else
        echo -e "${RED}✗ Outputs differ${NC}"
        echo -e "\n${BLUE}Differences (- Standard Lua, + RangeLua):${NC}"
        diff -u "$standard_lua_output" "$rangelua_output" || true

        echo -e "\n${BLUE}Standard Lua output:${NC}"
        cat "$standard_lua_output"
        echo -e "\n${BLUE}RangeLua output:${NC}"
        cat "$rangelua_output"
        return 1
    fi
}

echo -e "\n${YELLOW}=== Test Output ===${NC}"

# Run the test
if [ "$COMPARE" = true ]; then
    # Run comparison test
    if run_comparison_test; then
        echo -e "\n${GREEN}✓ Comparison test completed successfully${NC}"
        exit 0
    else
        echo -e "\n${RED}✗ Comparison test failed${NC}"
        exit 1
    fi
else
    # Run normal test
    if [ "$VERBOSE" = true ]; then
        echo -e "${BLUE}Command: xmake run rangelua ${CMD_ARGS[*]}${NC}"
    fi

    if xmake run rangelua "${CMD_ARGS[@]}"; then
        echo -e "\n${GREEN}✓ Test completed successfully${NC}"
        exit 0
    else
        echo -e "\n${RED}✗ Test failed${NC}"
        exit 1
    fi
fi
