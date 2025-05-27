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
    
    # Build and run the test
    if xmake build > /dev/null 2>&1 && xmake run rangelua --log-level=off "$full_path" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
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
            run_test "$category" "$filename"
        fi
    done
}

# Main execution
echo -e "${BLUE}RangeLua Progressive Test Suite${NC}"
echo -e "${BLUE}==============================${NC}"

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
run_category "01_basic"
run_category "02_types"
run_category "03_variables"
run_category "04_arithmetic"
run_category "05_logic"
run_category "06_control"

# Summary
echo -e "\n${YELLOW}=== Test Summary ===${NC}"
echo -e "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed. Check the output above for details.${NC}"
    exit 1
fi
