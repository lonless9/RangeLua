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
    echo "  -a, --all-debug         Enable debug logging for all modules"
    echo "  -v, --verbose           Show detailed output"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 examples/01_basic/hello.lua"
    echo "  $0 -m parser examples/02_types/numbers.lua"
    echo "  $0 -a examples/04_arithmetic/basic_math.lua"
    echo "  $0 -l debug examples/06_control/if_else.lua"
}

# Default values
MODULE=""
LOG_LEVEL=""
VERBOSE=false
ALL_DEBUG=false
TEST_FILE=""

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

if [ "$ALL_DEBUG" = true ]; then
    CMD_ARGS+=(--module-log "lexer:debug,parser:debug,codegen:debug,optimizer:debug,vm:debug,memory:debug,gc:debug")
elif [ -n "$MODULE" ]; then
    CMD_ARGS+=(--module-log "${MODULE}:debug")
fi

if [ -n "$LOG_LEVEL" ]; then
    CMD_ARGS+=(--log-level "$LOG_LEVEL")
fi

CMD_ARGS+=("$TEST_FILE")

# Show what we're running
echo -e "\n${BLUE}Running test with debugging:${NC}"
echo -e "${BLUE}File: $TEST_FILE${NC}"
if [ "$ALL_DEBUG" = true ]; then
    echo -e "${BLUE}Debug: All modules${NC}"
elif [ -n "$MODULE" ]; then
    echo -e "${BLUE}Debug: $MODULE module${NC}"
fi
if [ -n "$LOG_LEVEL" ]; then
    echo -e "${BLUE}Log Level: $LOG_LEVEL${NC}"
fi

echo -e "\n${YELLOW}=== Test Output ===${NC}"

# Run the test
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
