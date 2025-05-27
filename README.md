# RangeLua - Modern C++20 Lua 5.5 Interpreter

A high-performance Lua 5.5 interpreter implementation using modern C++20 features, advanced design patterns, and template metaprogramming.

## 🚀 Key Features

- **Complete Lua 5.5 Compatibility**: Full support for Lua 5.5 syntax and semantics
- **Modern C++20**: Leverages concepts, coroutines, ranges, constexpr, and designated initializers
- **Advanced Architecture**: Clean separation of concerns with proper abstraction layers
- **Design Patterns**: Implements Visitor, Strategy, Factory, Observer, Command, RAII, and Template Method patterns
- **Template Metaprogramming**: Type-safe metaprogramming with SFINAE/concepts and variadic templates
- **Zero-Cost Abstractions**: Compile-time optimizations and memory-efficient structures
- **Comprehensive Testing**: Unit tests, integration tests, and performance benchmarks
- **Debugging Support**: Built-in debugger, profiler, and extensive logging

## 🏗️ Architecture Overview

RangeLua follows a modern, modular architecture with strict separation of concerns:

### Core Design Principles

1. **Parser Separation**: Parser handles ONLY syntax analysis and AST construction
2. **CodeGen Control**: CodeGenerator has COMPLETE control over register allocation, jump lists, and bytecode emission
3. **Modern C++20**: Extensive use of concepts, coroutines, ranges, and constexpr
4. **Template Programming**: Advanced template techniques for type safety and performance
5. **Design Patterns**: Strategic application of proven design patterns
6. **Memory Safety**: RAII, smart pointers, and comprehensive error handling

### Module Responsibilities

- **Frontend**: Lexical analysis, parsing, and AST construction
- **Backend**: Code generation, optimization, and bytecode emission
- **Runtime**: Virtual machine execution, memory management, and garbage collection
- **API**: High-level C++ API for embedding and scripting
- **Utils**: Logging, profiling, debugging, and utility functions

## 📁 Project Structure

```
RangeLua/
├── include/rangelua/           # Public API headers
│   ├── core/                   # Core type definitions and concepts
│   │   ├── types.hpp          # Basic types and aliases
│   │   ├── concepts.hpp       # C++20 concepts for type safety
│   │   ├── error.hpp          # Error handling with modern features
│   │   ├── config.hpp         # Configuration constants
|   |   └── instruction.hpp    # Lua instruction definitions
│   ├── frontend/              # Lexer, Parser, AST
│   │   ├── lexer.hpp          # Lexical analyzer
│   │   ├── parser.hpp         # Recursive descent parser
│   │   └── ast.hpp            # Abstract syntax tree definitions
│   ├── backend/               # CodeGen, Optimizer, Bytecode
│   │   ├── codegen.hpp        # Code generator with register allocation
│   │   ├── optimizer.hpp      # Bytecode optimization passes
│   │   └── bytecode.hpp       # Bytecode instruction definitions
│   ├── runtime/               # VM execution, Memory management
│   │   ├── vm.hpp             # Virtual machine execution engine
│   │   ├── value.hpp          # Lua value system
│   │   ├── memory.hpp         # Memory management interfaces
│   │   └── gc.hpp             # Garbage collection
│   ├── api/                   # C++ API for embedding
│   │   ├── state.hpp          # Lua state management
│   │   ├── function.hpp       # Function wrappers
│   │   ├── table.hpp          # Table operations
│   │   └── coroutine.hpp      # Coroutine support
│   ├── utils/                 # Utilities and helpers
│   │   ├── logger.hpp         # Logging system (spdlog)
│   │   ├── profiler.hpp       # Performance profiling
│   │   └── debug.hpp          # Debug utilities
│   └── rangelua.hpp           # Main header (includes all components)
├── src/                       # Implementation files
│   ├── core/                  # Core implementations
│   ├── frontend/              # Frontend implementations
│   ├── backend/               # Backend implementations
│   ├── runtime/               # Runtime implementations
│   ├── api/                   # API implementations
│   ├── utils/                 # Utility implementations
│   └── main.cpp               # Main interpreter entry point
├── tests/                     # Unit and integration tests
├── benchmarks/                # Performance benchmarks
├── examples/                  # Example programs and demos
├── docs/                      # Documentation
├── tools/                     # Development tools
└── xmake.lua                  # Build configuration
```

## 🔧 Building the Project

RangeLua uses xmake as its build system with comprehensive configuration:

### Prerequisites

- **Compiler**: Clang with C++20 support (GCC 11+ or MSVC 2022+ also supported)
- **Build System**: xmake 2.7+
- **Dependencies**: spdlog, Catch2, Google Benchmark (automatically managed)

### Build Commands

```bash
# Install xmake (if not already installed)
# Linux/macOS:
curl -fsSL https://xmake.io/shget.text | bash
# Windows: See https://xmake.io/#/guide/installation

# Configure project
xmake config

# Build all targets
xmake build

# Build specific targets
xmake build rangelua          # Main interpreter
xmake build rangelua_test     # Unit tests
xmake build rangelua_benchmark # Benchmarks

# Run interpreter
xmake run rangelua [options] [script]

# Run tests
xmake run rangelua_test

# Run benchmarks
xmake run rangelua_benchmark

# Build examples
xmake build example_basic_usage
xmake build example_advanced_features
xmake build example_performance_demo
```

### Build Modes

```bash
# Debug mode (default) - with AddressSanitizer and debug symbols
xmake config -m debug
xmake build

# Release mode - optimized for performance
xmake config -m release
xmake build
```

## 🚀 Usage Examples

### Basic Usage

```cpp
#include <rangelua/rangelua.hpp>

int main() {
    // Initialize RangeLua
    rangelua::initialize();

    // Create Lua state
    rangelua::api::State state;

    // Execute Lua code
    auto result = state.execute(R"(
        local function factorial(n)
            return n <= 1 and 1 or n * factorial(n - 1)
        end
        return factorial(10)
    )");

    if (result) {
        std::cout << "Result: " << result.value()[0].debug_string() << std::endl;
    }

    rangelua::cleanup();
    return 0;
}
```

### Advanced Features

```cpp
#include <rangelua/rangelua.hpp>

int main() {
    rangelua::initialize();

    // Configure logging
    rangelua::utils::Logger::set_level(spdlog::level::debug);

    // Create state with custom configuration
    rangelua::api::State state;

    // Execute complex Lua code with tables and coroutines
    auto result = state.execute(R"(
        local co = coroutine.create(function()
            local data = {}
            for i = 1, 100 do
                data[i] = i * i
                if i % 10 == 0 then
                    coroutine.yield(i)
                end
            end
            return data
        end)

        local results = {}
        while coroutine.status(co) ~= "dead" do
            local ok, value = coroutine.resume(co)
            if ok and value then
                table.insert(results, value)
            end
        end

        return results
    )");

    rangelua::cleanup();
    return 0;
}
```

## 🧪 Testing

RangeLua includes comprehensive testing:

```bash
# Run all tests
xmake run rangelua_test

# Run specific test categories
xmake run rangelua_test "[core]"
xmake run rangelua_test "[runtime]"
xmake run rangelua_test "[integration]"

# Run with verbose output
xmake run rangelua_test -v high
```

## 📊 Benchmarking

Performance benchmarks are available:

```bash
# Run all benchmarks
xmake run rangelua_benchmark

# Run specific benchmarks
xmake run rangelua_benchmark --benchmark_filter="BM_ValueCreation"

# Generate benchmark report
xmake run rangelua_benchmark --benchmark_format=json > benchmark_results.json
```

## 🐛 Debugging and Logging

RangeLua provides an advanced, developer-friendly logging system with module-specific control:

### Enhanced Logging Features

- **Module-specific log levels**: Control logging for individual components (lexer, parser, codegen, optimizer, vm, memory, gc)
- **Command-line integration**: Easy configuration through command-line arguments
- **Catch2 test integration**: Debug-friendly testing with logging control
- **Multiple output formats**: Console and file output with customizable patterns

### Main Executable Logging

```bash
# Global log level control
xmake run rangelua --log-level debug script.lua
xmake run rangelua --log-level trace --interactive

# Module-specific logging
xmake run rangelua --module-log parser:debug --module-log lexer:trace script.lua
xmake run rangelua --module-log vm:info --module-log gc:warn script.lua

# File output
xmake run rangelua --log-file rangelua.log --log-level debug script.lua

# Combined usage
xmake run rangelua --log-level warn --module-log parser:trace --log-file debug.log script.lua
```

### Test Logging

```bash
# Global test logging
xmake run rangelua_test --test-log-level debug
xmake run rangelua_test --test-log-level info --test-log-file test.log

# Module-specific test logging
xmake run rangelua_test "[parser]" --test-module-log parser:trace
xmake run rangelua_test "[lexer]" --test-module-log lexer:debug --test-module-log parser:info

# Combined with Catch2 filters
xmake run rangelua_test "[integration]" --test-log-level off --test-module-log vm:debug
```

### Available Log Levels

- `trace`: Most verbose, shows all internal operations
- `debug`: Detailed debugging information
- `info`: General information messages
- `warn`: Warning messages for potential issues
- `error`: Error messages for failures
- `critical`: Critical errors that may cause termination
- `off`: Disable logging completely

### Available Modules

- `lexer`: Tokenization and lexical analysis
- `parser`: Syntax analysis and AST construction
- `codegen`: Code generation and bytecode emission
- `optimizer`: Bytecode optimization passes
- `vm`: Virtual machine execution
- `memory`: Memory management and allocation
- `gc`: Garbage collection operations

### Logging Demo

```bash
# Run the enhanced logging demonstration
xmake run enhanced_logging_demo
```

## 🎯 Design Goals Achieved

### Architectural Improvements

✅ **Strict Separation of Concerns**: Parser handles only syntax analysis; CodeGen manages all register allocation and bytecode emission

✅ **Modern C++20 Integration**: Comprehensive use of concepts, coroutines, ranges, constexpr, and designated initializers

✅ **Advanced Template Programming**: Type-safe metaprogramming with SFINAE/concepts and variadic templates

✅ **Design Pattern Implementation**: Strategic use of Visitor, Strategy, Factory, Observer, Command, RAII, and Template Method patterns

✅ **Performance Optimization**: Zero-cost abstractions, compile-time optimizations, and memory-efficient structures

### Technical Excellence

✅ **Register Allocation Control**: Complete encapsulation in CodeGenerator with high-level interfaces

✅ **Jump List Management**: Proper handling of forward/backward jumps and control flow structures

✅ **Memory Safety**: RAII principles, smart pointers, and comprehensive error handling

✅ **Error Handling**: Modern C++20 error handling with Result types and exception safety

✅ **Lua 5.5 Compatibility**: Maintains full compatibility with Lua 5.5 semantics

## 📚 Documentation

- **API Reference**: See `docs/api/` for detailed API documentation
- **Architecture Guide**: See `docs/architecture.md` for design decisions
- **Contributing**: See `docs/contributing.md` for development guidelines
- **Examples**: See `examples/` for usage examples

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](docs/contributing.md) for details on:

- Code style and conventions
- Testing requirements
- Pull request process
- Development setup

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
