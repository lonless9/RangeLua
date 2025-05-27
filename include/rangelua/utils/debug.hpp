#pragma once

/**
 * @file debug.hpp
 * @brief Debug utilities and assertions with modern C++20 features
 * @version 0.1.0
 */

#include <chrono>
#include <source_location>

#include "../core/config.hpp"
#include "../core/types.hpp"

namespace rangelua::utils {

    /**
     * @brief Debug utilities with enhanced functionality
     */
    class Debug {
    public:
        /**
         * @brief Assert with message and automatic source location
         */
        static void assert_msg(bool condition, const String& message);

        /**
         * @brief Print debug message with timestamp and thread info
         */
        static void print(const String& message);

        /**
         * @brief Set name for current thread (for debug output)
         */
        static void set_thread_name(const String& name);

        /**
         * @brief Start a debug timer
         */
        static void start_timer(const String& name);

        /**
         * @brief End a debug timer and return elapsed time
         */
        static std::chrono::nanoseconds end_timer(const String& name);

        /**
         * @brief Format memory size in human-readable format
         */
        static String format_memory_size(Size bytes);

        /**
         * @brief Dump current stack trace
         */
        static void dump_stack_trace();

        /**
         * @brief Check if debugging is enabled
         */
        static constexpr bool is_enabled() noexcept { return config::DEBUG_ENABLED; }

        /**
         * @brief Check if tracing is enabled
         */
        static constexpr bool is_trace_enabled() noexcept { return config::TRACE_ENABLED; }
    };

    /**
     * @brief RAII debug timer for automatic timing
     */
    class DebugTimer {
    public:
        explicit DebugTimer(String name) : name_(std::move(name)) { Debug::start_timer(name_); }

        ~DebugTimer() {
            auto duration = Debug::end_timer(name_);
            if constexpr (config::DEBUG_ENABLED) {
                Debug::print("Timer '" + name_ + "' elapsed: " + std::to_string(duration.count()) +
                             " ns");
            }
        }

        // Non-copyable, non-movable
        DebugTimer(const DebugTimer&) = delete;
        DebugTimer& operator=(const DebugTimer&) = delete;
        DebugTimer(DebugTimer&&) = delete;
        DebugTimer& operator=(DebugTimer&&) = delete;

    private:
        String name_;
    };

}  // namespace rangelua::utils

// Enhanced debug macros with modern C++20 features
#if RANGELUA_DEBUG
#    define RANGELUA_ASSERT(condition)                                                             \
        do {                                                                                       \
            if (!(condition)) {                                                                    \
                rangelua::utils::Debug::assert_msg(false, "Assertion failed: " #condition);        \
            }                                                                                      \
        } while (0)

#    define RANGELUA_ASSERT_MSG(condition, message)                                                \
        do {                                                                                       \
            if (!(condition)) {                                                                    \
                rangelua::utils::Debug::assert_msg(false, message);                                \
            }                                                                                      \
        } while (0)

#    define RANGELUA_DEBUG_PRINT(message) rangelua::utils::Debug::print(message)

#    define RANGELUA_DEBUG_TIMER(name) rangelua::utils::DebugTimer _debug_timer_##__LINE__(name)

#    define RANGELUA_TRACE_FUNCTION()                                                              \
        RANGELUA_DEBUG_PRINT("Entering function: " + String(__PRETTY_FUNCTION__))

#    define RANGELUA_DUMP_STACK() rangelua::utils::Debug::dump_stack_trace()

#    define RANGELUA_SET_THREAD_NAME(name) rangelua::utils::Debug::set_thread_name(name)

#    define RANGELUA_FORMAT_MEMORY(bytes) rangelua::utils::Debug::format_memory_size(bytes)

#else
#    define RANGELUA_ASSERT(condition)              ((void)0)
#    define RANGELUA_ASSERT_MSG(condition, message) ((void)0)
#    define RANGELUA_DEBUG_PRINT(message)           ((void)0)
#    define RANGELUA_DEBUG_TIMER(name)              ((void)0)
#    define RANGELUA_TRACE_FUNCTION()               ((void)0)
#    define RANGELUA_DUMP_STACK()                   ((void)0)
#    define RANGELUA_SET_THREAD_NAME(name)          ((void)0)
#    define RANGELUA_FORMAT_MEMORY(bytes)                                                          \
        String {}
#endif

// Conditional debug macros that work in both debug and release
#define RANGELUA_DEBUG_IF(condition, message)                                                      \
    do {                                                                                           \
        if constexpr (rangelua::config::DEBUG_ENABLED) {                                           \
            if (condition) {                                                                       \
                rangelua::utils::Debug::print(message);                                            \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define RANGELUA_TRACE_IF(condition, message)                                                      \
    do {                                                                                           \
        if constexpr (rangelua::config::TRACE_ENABLED) {                                           \
            if (condition) {                                                                       \
                rangelua::utils::Debug::print("[TRACE] " + String(message));                       \
            }                                                                                      \
        }                                                                                          \
    } while (0)
