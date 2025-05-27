# RangeLua Test Suite

This directory contains a comprehensive test-driven development suite for RangeLua, organized progressively from simple to complex functionality.

## Directory Structure

### 01_basic/ - Basic Output
Tests the most fundamental functionality - printing output.
- `hello.lua` - Simple print statement
- `multiple_prints.lua` - Multiple sequential prints
- `print_multiple_args.lua` - Print with multiple arguments

### 02_types/ - Basic Data Types
Tests Lua's basic data types and the type system.
- `numbers.lua` - Integer and floating-point literals
- `strings.lua` - String literals and handling
- `booleans.lua` - Boolean values (true/false)
- `nil.lua` - Nil value handling
- `type_function.lua` - Testing the type() function

### 03_variables/ - Variables and Assignment
Tests variable declaration, assignment, and scoping.
- `local_vars.lua` - Local variable declaration
- `global_vars.lua` - Global variable usage
- `reassignment.lua` - Variable reassignment

### 04_arithmetic/ - Arithmetic Operations
Tests mathematical operations and expressions.
- `basic_math.lua` - Addition, subtraction, multiplication, division
- `modulo.lua` - Modulo operations
- `power.lua` - Exponentiation
- `unary.lua` - Unary minus and plus

### 05_logic/ - Logical Operations
Tests comparison and boolean operations.
- `comparison.lua` - Comparison operators (==, ~=, <, >, <=, >=)
- `boolean_ops.lua` - Boolean operators (and, or, not)
- `truthiness.lua` - Lua's truthiness rules

### 06_control/ - Control Flow
Tests conditional statements and loops.
- `if_else.lua` - Conditional statements
- `while.lua` - While loops
- `for_numeric.lua` - Numeric for loops
- `for_generic.lua` - Generic for loops (requires tables)

## Usage

### Running Individual Tests

To test a specific script:
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

## Progressive Testing Strategy

1. **Start with 01_basic/** - Ensure basic output works
2. **Move to 02_types/** - Verify data type handling
3. **Test 03_variables/** - Check variable assignment
4. **Validate 04_arithmetic/** - Confirm mathematical operations
5. **Verify 05_logic/** - Test logical operations
6. **Complete with 06_control/** - Validate control flow

Each category builds upon the previous ones, allowing you to identify and fix issues systematically.

## Expected Behavior

Each test file includes comments describing:
- What is being tested
- Expected output
- Description of the functionality

If a test fails, check:
1. Lexer - Is the syntax being parsed correctly?
2. Parser - Is the AST being built properly?
3. CodeGen - Is bytecode being generated correctly?
4. VM - Is the bytecode being executed properly?

## Adding New Tests

When adding new functionality to RangeLua:
1. Create appropriate test scripts in the relevant category
2. Include clear comments about expected behavior
3. Test incrementally to isolate issues
4. Use logging to debug specific modules

This test suite serves as both validation and documentation of RangeLua's capabilities.
