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
#include "core/concepts.hpp"
#include "core/config.hpp"
#include "core/error.hpp"
#include "core/types.hpp"

// Frontend components
#include "frontend/ast.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"

// Backend components
#include "backend/bytecode.hpp"
#include "backend/codegen.hpp"
#include "backend/optimizer.hpp"

// Runtime components
#include "runtime/gc.hpp"
#include "runtime/memory.hpp"
#include "runtime/value.hpp"
#include "runtime/vm.hpp"

// API components
#include "api/coroutine.hpp"
#include "api/function.hpp"
#include "api/state.hpp"
#include "api/table.hpp"

// Utility components
#include "utils/debug.hpp"
#include "utils/logger.hpp"
#include "utils/profiler.hpp"

namespace rangelua {

    // Forward declarations
    namespace runtime {
        class MemoryManager;
    }

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
     * @brief Initialize RangeLua library with modern error handling
     * @return Status indicating success or failure
     */
    Status initialize() noexcept;

    /**
     * @brief Cleanup RangeLua library
     */
    void cleanup() noexcept;

    /**
     * @brief Check if RangeLua is initialized
     * @return true if initialized, false otherwise
     */
    bool is_initialized() noexcept;

    /**
     * @brief Get the global memory manager instance
     * @return Pointer to memory manager or nullptr if not initialized
     */
    runtime::MemoryManager* get_memory_manager() noexcept;

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

}  // namespace rangelua
