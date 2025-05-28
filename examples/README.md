# RangeLua Test Suite

This directory contains test scripts and examples for RangeLua, organized progressively from basic to advanced functionality.

## Directory Structure

- `01_basic/` - Basic printing and hello world
- `02_types/` - Type system and basic values
- `03_variables/` - Variable declarations and assignments
- `04_arithmetic/` - Arithmetic operations
- `05_logic/` - Logical operations and comparisons
- `06_control/` - Control flow (if/else, loops)
- `advance/` - Comprehensive integration tests
- `stdlib/` - Standard library function tests
- `debug/` - Debug and development test scripts

## Test Scripts

### `run_tests.sh` - Progressive Test Suite
Run all tests in order:
```bash
./examples/run_tests.sh
```

Run tests with comparison to standard Lua 5.5:
```bash
./examples/run_tests.sh -c
```

Run comparison tests with verbose output:
```bash
./examples/run_tests.sh -c -v
```

### `compare_test.sh` - Comparison Testing
Compare RangeLua output with standard Lua 5.5:

Single file comparison:
```bash
./examples/compare_test.sh examples/stdlib/test_basic.lua
```

Directory comparison:
```bash
./examples/compare_test.sh examples/stdlib/
```

Verbose comparison (shows all outputs):
```bash
./examples/compare_test.sh -v examples/advance/
```

### `debug_test.sh` - Individual Test Debugging
Debug a specific test with detailed logging:
```bash
./examples/debug_test.sh examples/01_basic/hello.lua
```

Compare single test with standard Lua:
```bash
./examples/debug_test.sh -c examples/debug/simple_test.lua
```

With module-specific debugging:
```bash
./examples/debug_test.sh -m parser examples/02_types/numbers.lua
./examples/debug_test.sh -a examples/04_arithmetic/basic_math.lua  # All modules
```

## Manual Testing

### Running Individual Tests

To test a specific script manually:
```bash
xmake build && xmake run rangelua ./examples/[category]/[script].lua
```

Examples:
```bash
# Test basic hello world
xmake build && xmake run rangelua ./examples/01_basic/hello.lua

# Test number handling
xmake build && xmake run rangelua ./examples/02_types/numbers.lua

# Test arithmetic operations
xmake build && xmake run rangelua ./examples/04_arithmetic/basic_math.lua
```

### Running with Logging

For debugging, you can enable specific module logging:
```bash
# Enable parser debugging
xmake build && xmake run rangelua --module-log parser:debug ./examples/01_basic/hello.lua

# Enable all debugging
xmake build && xmake run rangelua --log-level=debug ./examples/01_basic/hello.lua

# Quiet mode (no logs)
xmake build && xmake run rangelua --log-level=off ./examples/01_basic/hello.lua
```

## Comparison Testing

The comparison testing feature allows you to verify that RangeLua produces the same output as standard Lua 5.5:

- **Automatic log filtering**: RangeLua debug logs are automatically filtered out during comparison
- **Detailed diff output**: Shows exactly where outputs differ
- **Batch testing**: Can test entire directories at once
- **Standard Lua location**: Uses `~/code/cpp/lua/lua` as the reference implementation

### Key Features

1. **Clean Output Comparison**: Automatically filters out RangeLua's debug logs
2. **Detailed Differences**: Shows unified diff format for easy analysis
3. **Batch Processing**: Test entire directories or individual files
4. **Verbose Mode**: Option to see all outputs, not just differences
5. **Stop on Failure**: Option to halt testing on first mismatch

## Test Categories

### Basic Tests (01_basic)
- Hello world printing
- Basic output functionality

### Type Tests (02_types)
- Numbers (integers and floats)
- Strings, booleans, nil values
- Type checking functions

### Variable Tests (03_variables)
- Global and local variable declarations
- Variable scoping and assignments

### Arithmetic Tests (04_arithmetic)
- Basic math operations (+, -, *, /, %)
- Operator precedence

### Logic Tests (05_logic)
- Boolean operations (and, or, not)
- Comparison operators

### Control Flow Tests (06_control)
- If/else statements
- While and for loops

### Advanced Tests (advance/)
- Comprehensive integration tests
- Mixed functionality scenarios

### Standard Library Tests (stdlib/)
- Basic library functions (print, type, tostring, etc.)
- Table operations and iteration
- String and math functions

## Debugging Tips

1. Use `debug_test.sh` for detailed output and logging
2. Enable specific module debugging with `-m` flag
3. Use comparison mode (`-c`) to verify compatibility with standard Lua
4. Check bytecode generation with codegen module debugging
5. Monitor VM execution with vm module debugging

## Writing New Tests

1. Choose the appropriate category directory
2. Create a `.lua` file with descriptive name
3. Add comments explaining expected behavior
4. Test with both `debug_test.sh` and comparison mode
5. Verify compatibility with standard Lua using `compare_test.sh`
