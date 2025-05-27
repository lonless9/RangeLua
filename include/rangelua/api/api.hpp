#pragma once

/**
 * @file api.hpp
 * @brief Main API header for RangeLua - includes all API components
 * @version 0.1.0
 *
 * This is the main convenience header that includes all RangeLua API components.
 * Include this header to get access to the complete high-level C++ API for
 * embedding RangeLua in your applications.
 */

// Core API components
#include "state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "coroutine.hpp"
#include "userdata.hpp"

// Runtime integration
#include "../runtime/value.hpp"
#include "../runtime/objects.hpp"

namespace rangelua::api {

    /**
     * @brief API version information
     */
    struct Version {
        static constexpr int MAJOR = 0;
        static constexpr int MINOR = 1;
        static constexpr int PATCH = 0;
        static constexpr const char* STRING = "0.1.0";
        static constexpr const char* NAME = "RangeLua";
    };

    /**
     * @brief Initialize the RangeLua API
     *
     * This function should be called once before using any RangeLua API functions.
     * It sets up global state and initializes subsystems.
     */
    void initialize();

    /**
     * @brief Cleanup the RangeLua API
     *
     * This function should be called once when done using RangeLua API.
     * It cleans up global state and shuts down subsystems.
     */
    void cleanup();

    /**
     * @brief Get API version information
     */
    [[nodiscard]] const Version& version() noexcept;

    /**
     * @brief Convenience functions for common operations
     */
    namespace convenience {

        /**
         * @brief Execute Lua code and return the first result
         */
        template<typename T = runtime::Value>
        [[nodiscard]] Result<T> eval(const String& code);

        /**
         * @brief Execute Lua file and return the first result
         */
        template<typename T = runtime::Value>
        [[nodiscard]] Result<T> eval_file(const String& filename);

        /**
         * @brief Create a new Lua state with default configuration
         */
        [[nodiscard]] State create_state();

        /**
         * @brief Create a new Lua state with custom configuration
         */
        [[nodiscard]] State create_state(const StateConfig& config);

        /**
         * @brief Create a table from initializer list
         */
        [[nodiscard]] Table create_table(std::initializer_list<std::pair<runtime::Value, runtime::Value>> init = {});

        /**
         * @brief Create a function from C++ callable
         */
        template<typename Callable>
        [[nodiscard]] Function create_function(Callable&& callable);

        /**
         * @brief Create a coroutine with default stack size
         */
        [[nodiscard]] Coroutine create_coroutine();

        /**
         * @brief Create a coroutine with specified stack size
         */
        [[nodiscard]] Coroutine create_coroutine(Size stack_size);

        /**
         * @brief Create userdata with specified size
         */
        [[nodiscard]] Userdata create_userdata(Size size);

        /**
         * @brief Create userdata from C++ object
         */
        template<typename T>
        [[nodiscard]] Userdata create_userdata(T&& object);

    }  // namespace convenience

    /**
     * @brief Type conversion utilities
     */
    namespace convert {

        /**
         * @brief Convert C++ value to Lua Value
         */
        template<typename T>
        [[nodiscard]] runtime::Value to_lua(T&& value);

        /**
         * @brief Convert Lua Value to C++ type
         */
        template<typename T>
        [[nodiscard]] Result<T> from_lua(const runtime::Value& value);

        /**
         * @brief Convert C++ container to Lua table
         */
        template<typename Container>
        [[nodiscard]] Table container_to_table(const Container& container);

        /**
         * @brief Convert Lua table to C++ container
         */
        template<typename Container>
        [[nodiscard]] Result<Container> table_to_container(const Table& table);

    }  // namespace convert

    /**
     * @brief Error handling utilities
     */
    namespace error {

        /**
         * @brief Convert ErrorCode to string
         */
        [[nodiscard]] String to_string(ErrorCode code);

        /**
         * @brief Check if Result contains success value
         */
        template<typename T>
        [[nodiscard]] bool is_success(const Result<T>& result);

        /**
         * @brief Get success value from Result (throws on error)
         */
        template<typename T>
        [[nodiscard]] const T& get_value(const Result<T>& result);

        /**
         * @brief Get error code from Result (throws on success)
         */
        template<typename T>
        [[nodiscard]] ErrorCode get_error(const Result<T>& result);

        /**
         * @brief Get value or default on error
         */
        template<typename T>
        [[nodiscard]] T get_value_or(const Result<T>& result, T&& default_value);

    }  // namespace error

    /**
     * @brief Debugging and introspection utilities
     */
    namespace debug {

        /**
         * @brief Get debug string representation of a Value
         */
        [[nodiscard]] String value_to_string(const runtime::Value& value);

        /**
         * @brief Get type name of a Value
         */
        [[nodiscard]] String value_type_name(const runtime::Value& value);

        /**
         * @brief Print Value to stdout (for debugging)
         */
        void print_value(const runtime::Value& value);

        /**
         * @brief Dump table contents (for debugging)
         */
        void dump_table(const Table& table, Size max_depth = 3);

    }  // namespace debug

}  // namespace rangelua::api

/**
 * @brief Global convenience macros for common operations
 */
#define RANGELUA_VERSION_STRING rangelua::api::Version::STRING
#define RANGELUA_VERSION_MAJOR rangelua::api::Version::MAJOR
#define RANGELUA_VERSION_MINOR rangelua::api::Version::MINOR
#define RANGELUA_VERSION_PATCH rangelua::api::Version::PATCH

/**
 * @brief Convenience macro for error checking
 */
#define RANGELUA_CHECK(result) \
    do { \
        if (!rangelua::api::error::is_success(result)) { \
            throw std::runtime_error("RangeLua error: " + rangelua::api::error::to_string(rangelua::api::error::get_error(result))); \
        } \
    } while(0)

/**
 * @brief Convenience macro for getting value with error checking
 */
#define RANGELUA_GET(result) \
    (RANGELUA_CHECK(result), rangelua::api::error::get_value(result))
