#pragma once

/**
 * @file config.hpp
 * @brief Configuration constants and compile-time settings
 * @version 0.1.0
 */

#include <limits>

#include "types.hpp"

namespace rangelua::config {

    // Version information
    constexpr int VERSION_MAJOR = 0;
    constexpr int VERSION_MINOR = 1;
    constexpr int VERSION_PATCH = 0;
    constexpr const char* VERSION_STRING = "0.1.0";
    constexpr const char* LUA_VERSION = "5.5";

    // VM configuration
    constexpr Size DEFAULT_STACK_SIZE = 1024;
    constexpr Size MAX_STACK_SIZE = 1024 * 1024;
    constexpr Size DEFAULT_CALL_STACK_SIZE = 256;
    constexpr Size MAX_CALL_STACK_SIZE = 8192;

    // Memory configuration
    constexpr Size DEFAULT_HEAP_SIZE = 64 * 1024 * 1024;       // 64MB
    constexpr Size MAX_HEAP_SIZE = 2ULL * 1024 * 1024 * 1024;  // 2GB
    constexpr Size GC_THRESHOLD = 8 * 1024 * 1024;             // 8MB
    constexpr double GC_MULTIPLIER = 2.0;
    constexpr Size ALLOCATION_ALIGNMENT = 8;

    // String configuration
    constexpr Size MAX_STRING_LENGTH = 1024 * 1024;  // 1MB
    constexpr Size STRING_CACHE_SIZE = 256;
    constexpr Size SHORT_STRING_LIMIT = 40;

    // Table configuration
    constexpr Size DEFAULT_TABLE_SIZE = 16;
    constexpr Size MAX_TABLE_SIZE = 1024 * 1024;
    constexpr double TABLE_LOAD_FACTOR = 0.75;

    // Function configuration
    constexpr Size MAX_FUNCTION_PARAMS = 255;
    constexpr Size MAX_UPVALUES = 255;
    constexpr Size MAX_LOCALS = 255;
    constexpr Size MAX_CONSTANTS = 65536;

    // Instruction configuration
    constexpr Size MAX_INSTRUCTIONS = 1024 * 1024;
    constexpr Size MAX_JUMP_DISTANCE = 32767;
    constexpr Register MAX_REGISTERS = 255;

    // Parser configuration
    constexpr Size MAX_PARSE_DEPTH = 1000;
    constexpr Size MAX_EXPRESSION_DEPTH = 200;
    constexpr Size TOKEN_BUFFER_SIZE = 1024;

    // Lexer configuration
    constexpr Size MAX_TOKEN_LENGTH = 1024;
    constexpr Size LEXER_BUFFER_SIZE = 4096;

    // Error configuration
    constexpr Size MAX_ERROR_MESSAGE_LENGTH = 1024;
    constexpr Size MAX_STACK_TRACE_DEPTH = 100;

// Debug configuration
#ifdef RANGELUA_DEBUG
    constexpr bool DEBUG_ENABLED = true;
    constexpr bool TRACE_ENABLED = true;
    constexpr bool ASSERT_ENABLED = true;
#else
    constexpr bool DEBUG_ENABLED = false;
    constexpr bool TRACE_ENABLED = false;
    constexpr bool ASSERT_ENABLED = false;
#endif

    // Optimization configuration
    constexpr bool CONSTANT_FOLDING_ENABLED = true;
    constexpr bool DEAD_CODE_ELIMINATION_ENABLED = true;
    constexpr bool TAIL_CALL_OPTIMIZATION_ENABLED = true;
    constexpr bool REGISTER_ALLOCATION_OPTIMIZATION = true;

    // Coroutine configuration
    constexpr Size DEFAULT_COROUTINE_STACK_SIZE = 64 * 1024;  // 64KB
    constexpr Size MAX_COROUTINES = 10000;

    // I/O configuration
    constexpr Size IO_BUFFER_SIZE = 8192;
    constexpr Size MAX_FILE_SIZE = 100 * 1024 * 1024;  // 100MB

    // Numeric limits
    constexpr Int MIN_INTEGER = std::numeric_limits<Int>::min();
    constexpr Int MAX_INTEGER = std::numeric_limits<Int>::max();
    constexpr Number MIN_NUMBER = std::numeric_limits<Number>::lowest();
    constexpr Number MAX_NUMBER = std::numeric_limits<Number>::max();
    constexpr Number NUMBER_EPSILON = std::numeric_limits<Number>::epsilon();

    // Feature flags
    constexpr bool ENABLE_JIT = false;  // Future JIT compilation support
    constexpr bool ENABLE_PROFILING = true;
    constexpr bool ENABLE_DEBUGGING = true;
    constexpr bool ENABLE_COROUTINES = true;
    constexpr bool ENABLE_MODULES = true;
    constexpr bool ENABLE_FFI = false;  // Future FFI support

// Platform-specific configuration
#ifdef _WIN32
    constexpr bool WINDOWS_PLATFORM = true;
    constexpr const char* PATH_SEPARATOR = "\\";
    constexpr const char* LINE_ENDING = "\r\n";
#else
    constexpr bool WINDOWS_PLATFORM = false;
    constexpr const char* PATH_SEPARATOR = "/";
    constexpr const char* LINE_ENDING = "\n";
#endif

// Compiler-specific configuration
#ifdef __clang__
    constexpr bool CLANG_COMPILER = true;
#else
    constexpr bool CLANG_COMPILER = false;
#endif

#ifdef __GNUC__
    constexpr bool GCC_COMPILER = true;
#else
    constexpr bool GCC_COMPILER = false;
#endif

#ifdef _MSC_VER
    constexpr bool MSVC_COMPILER = true;
#else
    constexpr bool MSVC_COMPILER = false;
#endif

// C++20 feature detection
#ifdef __cpp_concepts
    constexpr bool CONCEPTS_AVAILABLE = true;
#else
    constexpr bool CONCEPTS_AVAILABLE = false;
#endif

#ifdef __cpp_coroutines
    constexpr bool COROUTINES_AVAILABLE = true;
#else
    constexpr bool COROUTINES_AVAILABLE = false;
#endif

#ifdef __cpp_modules
    constexpr bool MODULES_AVAILABLE = true;
#else
    constexpr bool MODULES_AVAILABLE = false;
#endif

    // Logging configuration
    enum class LogLevel : int {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Critical = 5,
        Off = 6
    };

#ifdef RANGELUA_DEBUG
    constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::Debug;
#else
    constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::Info;
#endif

    constexpr Size LOG_BUFFER_SIZE = 1024;
    constexpr const char* LOG_FORMAT = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v";

    // Performance tuning
    constexpr bool INLINE_SMALL_FUNCTIONS = true;
    constexpr bool USE_COMPUTED_GOTO = true;  // For VM dispatch
    constexpr bool PREFETCH_INSTRUCTIONS = true;
    constexpr Size INSTRUCTION_CACHE_SIZE = 1024;

    // Security configuration
    constexpr Size MAX_RECURSION_DEPTH = 1000;
    constexpr Size MAX_MEMORY_PER_STATE = 1024 * 1024 * 1024;  // 1GB
    constexpr bool SANDBOX_MODE = false;

    // Compatibility configuration
    constexpr bool LUA_COMPAT_5_1 = false;
    constexpr bool LUA_COMPAT_5_2 = false;
    constexpr bool LUA_COMPAT_5_3 = true;
    constexpr bool LUA_COMPAT_5_4 = true;

}  // namespace rangelua::config
