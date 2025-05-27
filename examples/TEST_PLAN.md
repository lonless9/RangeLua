# RangeLua Test-Driven Development Plan

## Overview

This test suite provides a systematic approach to validating and debugging RangeLua implementation. The tests are organized progressively to isolate issues at each level of the interpreter stack.

## Testing Strategy

### Phase 1: Foundation (01_basic, 02_types)
**Goal**: Verify basic lexing, parsing, and value system

**Critical Tests**:
- `01_basic/hello.lua` - Minimal functionality test
- `02_types/numbers.lua` - Number literal parsing
- `02_types/strings.lua` - String literal parsing
- `02_types/type_function.lua` - Basic function calls

**Common Issues to Watch For**:
- Lexer not recognizing tokens correctly
- Parser failing to build AST
- Value system not handling type conversions
- Print function not working

**Debugging Commands**:
```bash
# Test lexer specifically
./examples/debug_test.sh -m lexer examples/01_basic/hello.lua

# Test parser specifically  
./examples/debug_test.sh -m parser examples/02_types/numbers.lua

# Full debugging
./examples/debug_test.sh -a examples/01_basic/hello.lua
```

### Phase 2: Variables and Expressions (03_variables, 04_arithmetic)
**Goal**: Verify variable handling and expression evaluation

**Critical Tests**:
- `03_variables/local_vars.lua` - Local variable scoping
- `04_arithmetic/basic_math.lua` - Expression evaluation

**Common Issues to Watch For**:
- Variable scoping problems
- Arithmetic operations not implemented
- CodeGen not generating correct bytecode
- VM not executing arithmetic instructions

**Debugging Commands**:
```bash
# Test codegen
./examples/debug_test.sh -m codegen examples/04_arithmetic/basic_math.lua

# Test VM execution
./examples/debug_test.sh -m vm examples/03_variables/local_vars.lua
```

### Phase 3: Logic and Control Flow (05_logic, 06_control)
**Goal**: Verify logical operations and control structures

**Critical Tests**:
- `05_logic/comparison.lua` - Comparison operators
- `06_control/if_else.lua` - Conditional execution
- `06_control/while.lua` - Loop execution

**Common Issues to Watch For**:
- Comparison operators not working
- Jump instructions not implemented
- Control flow bytecode generation issues
- Loop termination problems

## Debugging Workflow

### 1. Start with Minimal Test
```bash
./examples/debug_test.sh examples/debug/minimal.lua
```

### 2. Identify the Failing Component
- **Lexer Issues**: Tokens not recognized, syntax errors
- **Parser Issues**: AST construction failures
- **CodeGen Issues**: Bytecode generation problems
- **VM Issues**: Execution failures, runtime errors

### 3. Use Targeted Debugging
```bash
# For lexer issues
./examples/debug_test.sh -m lexer examples/debug/lexer_test.lua

# For parser issues
./examples/debug_test.sh -m parser examples/02_types/numbers.lua

# For codegen issues
./examples/debug_test.sh -m codegen examples/04_arithmetic/basic_math.lua

# For VM issues
./examples/debug_test.sh -m vm examples/01_basic/hello.lua
```

### 4. Progressive Testing
Run tests in order, fixing issues as they appear:
```bash
# Run all tests
./examples/run_tests.sh

# Or run category by category
./examples/debug_test.sh examples/01_basic/hello.lua
./examples/debug_test.sh examples/02_types/numbers.lua
# ... continue until failure
```

## Expected Implementation Order

Based on Lua interpreter architecture, implement and test in this order:

1. **Lexer** - Token recognition
2. **Parser** - AST construction  
3. **Value System** - Basic data types
4. **CodeGen** - Bytecode generation for literals
5. **VM** - Basic instruction execution
6. **Variables** - Local/global variable handling
7. **Expressions** - Arithmetic and logical operations
8. **Control Flow** - Conditionals and loops

## Success Criteria

### Milestone 1: Basic Functionality
- All tests in `01_basic/` pass
- All tests in `02_types/` pass
- Can print literals and call basic functions

### Milestone 2: Variables and Expressions  
- All tests in `03_variables/` pass
- All tests in `04_arithmetic/` pass
- Can handle variable assignment and arithmetic

### Milestone 3: Complete Basic Interpreter
- All tests in `05_logic/` pass
- All tests in `06_control/` pass
- Can execute conditional statements and loops

## Troubleshooting Guide

### Common Error Patterns

**"Segmentation Fault"**
- Check memory management in Value system
- Verify GC pointer handling
- Use AddressSanitizer: `xmake config -m debug`

**"Unknown Instruction"**
- CodeGen not generating correct bytecode
- VM missing instruction implementations
- Check instruction encoding/decoding

**"Type Error"**
- Value system type checking issues
- Incorrect type coercion
- Missing type conversion methods

**"Parse Error"**
- Lexer not recognizing tokens
- Parser grammar issues
- AST construction problems

### Debug Output Analysis

Look for these patterns in debug output:

**Lexer Debug**:
- Token types and values
- Position information
- Lexical errors

**Parser Debug**:
- AST node creation
- Grammar rule matching
- Parse tree structure

**CodeGen Debug**:
- Bytecode instruction generation
- Register allocation
- Constant pool management

**VM Debug**:
- Instruction execution
- Stack operations
- Control flow changes

This systematic approach will help identify and fix issues efficiently while building RangeLua incrementally.
