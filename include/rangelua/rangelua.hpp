#pragma once

/**
 * @file rangelua.hpp
 * @brief Main RangeLua header - includes all public API components
 * @version 0.1.0
 * 
 * RangeLua - A modern C++20 Lua 5.5 interpreter implementation
 * 
 * This header provides the complete public API for RangeLua, including:
 * - Core VM functionality
 * - Frontend parsing and lexing
 * - Backend code generation
 * - Runtime execution and memory management
 * - C++ API for embedding
 * 
 * @copyright MIT License
 */

// Core components
#include "core/types.hpp"
#include "core/concepts.hpp"
#include "core/error.hpp"
#include "core/config.hpp"

// Frontend components
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/ast.hpp"

// Backend components
#include "backend/codegen.hpp"
#include "backend/optimizer.hpp"
#include "backend/bytecode.hpp"

// Runtime components
#include "runtime/vm.hpp"
#include "runtime/memory.hpp"
#include "runtime/value.hpp"
#include "runtime/gc.hpp"

// API components
#include "api/state.hpp"
#include "api/function.hpp"
#include "api/table.hpp"
#include "api/coroutine.hpp"

// Utility components
#include "utils/logger.hpp"
#include "utils/profiler.hpp"
#include "utils/debug.hpp"

namespace rangelua {

/**
 * @brief RangeLua version information
 */
struct Version {
    static constexpr int major = 0;
    static constexpr int minor = 1;
    static constexpr int patch = 0;
    static constexpr const char* string = "0.1.0";
    static constexpr const char* lua_version = "5.5";
};

/**
 * @brief Initialize RangeLua library
 * @return true if initialization successful, false otherwise
 */
bool initialize() noexcept;

/**
 * @brief Cleanup RangeLua library
 */
void cleanup() noexcept;

/**
 * @brief Get RangeLua version string
 * @return Version string
 */
constexpr const char* version() noexcept {
    return Version::string;
}

/**
 * @brief Get compatible Lua version string
 * @return Lua version string
 */
constexpr const char* lua_version() noexcept {
    return Version::lua_version;
}

} // namespace rangelua
