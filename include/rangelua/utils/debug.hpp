#pragma once

/**
 * @file debug.hpp
 * @brief Debug utilities and assertions
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../core/config.hpp"

namespace rangelua::utils {

/**
 * @brief Debug utilities
 */
class Debug {
public:
    /**
     * @brief Assert with message
     */
    static void assert_msg(bool condition, const String& message);
    
    /**
     * @brief Print debug message
     */
    static void print(const String& message);
    
    /**
     * @brief Check if debugging is enabled
     */
    static constexpr bool is_enabled() noexcept {
        return config::DEBUG_ENABLED;
    }
};

} // namespace rangelua::utils

// Debug macros
#if RANGELUA_DEBUG
#define RANGELUA_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            rangelua::utils::Debug::assert_msg(false, "Assertion failed: " #condition); \
        } \
    } while(0)

#define RANGELUA_ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            rangelua::utils::Debug::assert_msg(false, message); \
        } \
    } while(0)

#define RANGELUA_DEBUG_PRINT(message) \
    rangelua::utils::Debug::print(message)
#else
#define RANGELUA_ASSERT(condition) ((void)0)
#define RANGELUA_ASSERT_MSG(condition, message) ((void)0)
#define RANGELUA_DEBUG_PRINT(message) ((void)0)
#endif
