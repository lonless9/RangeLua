# RangeLua ⚡
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/lonless9/RangeLua)

A Lua 5.5 JIT interpreter implementation in C++20.

## ⚠️ Important Notice

**This is an experimental implementation following the Lua 5.5 development.**

🔬 Lua 5.5 itself is still under active development by the official Lua team, and this project is an immature implementation that follows the evolving specification. Expect breaking changes and incomplete features as both the official Lua 5.5 and this implementation continue to evolve.

## ✨ Features

- 🌟 Lua 5.5 compatibility
- ⚡ C++20 implementation
- 📝 Module-specific logging
- 🔍 Comparison testing with official Lua 5.5

## 🔨 Build

Prerequisites:
- 🛠️ Clang with C++20 support
- 📦 xmake 2.7+

```bash
xmake build
xmake run rangelua script.lua
```

## 🚀 Usage

```bash
# Run script
xmake run rangelua script.lua

# Interactive mode
xmake run rangelua -i

# With logging
xmake run rangelua --log-level debug script.lua
xmake run rangelua --module-log parser:debug script.lua
```

## 🧪 Testing

```bash
# Run all tests
./tests/run_tests.sh

# Compare with Lua 5.5
./tests/run_tests.sh --compare

# Debug single file
./tests/debug_test.sh tests/advance/comprehensive_basic.lua
```

## 📊 Status

### ✅ Implemented
- 🔤 Lexer, parser, ast, code generation
- 🖥️ Virtual machine execution
- 🔧 Metamethods
- 📚 Basic standard library functions
- 📝 Module-specific logging

### 🚧 In Development
- 📖 Other stdlib
- 🔄 Coroutines
- ⚠️ Error handling improvements

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
