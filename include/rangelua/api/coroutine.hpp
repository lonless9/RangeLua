#pragma once

/**
 * @file coroutine.hpp
 * @brief Comprehensive Coroutine API for RangeLua
 * @version 0.1.0
 *
 * Provides a high-level C++ interface for Lua coroutines with RAII semantics,
 * type safety, and integration with the runtime objects system.
 */

#include <vector>

#include "../core/types.hpp"
#include "../runtime/objects.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

    /**
     * @brief High-level Coroutine wrapper with comprehensive API
     *
     * Provides a safe, convenient interface for working with Lua coroutines.
     * Automatically manages the underlying runtime::Coroutine object and provides
     * type-safe operations with proper error handling.
     */
    class Coroutine {
    public:
        // Coroutine status enumeration
        enum class Status : std::uint8_t {
            SUSPENDED,  // Coroutine is suspended (can be resumed)
            RUNNING,    // Coroutine is currently running
            NORMAL,     // Coroutine is active but not running (calling another coroutine)
            DEAD        // Coroutine has finished or encountered an error
        };

        // Construction and conversion
        explicit Coroutine();
        explicit Coroutine(runtime::Value value);
        explicit Coroutine(runtime::GCPtr<runtime::Coroutine> coroutine);
        explicit Coroutine(Size stack_size);

        // Copy and move semantics
        Coroutine(const Coroutine& other) = default;
        Coroutine& operator=(const Coroutine& other) = default;
        Coroutine(Coroutine&& other) noexcept = default;
        Coroutine& operator=(Coroutine&& other) noexcept = default;

        ~Coroutine() = default;

        // Validation
        [[nodiscard]] bool is_valid() const noexcept;
        [[nodiscard]] bool is_coroutine() const noexcept;

        // Coroutine status
        [[nodiscard]] Status status() const;
        [[nodiscard]] bool is_suspended() const noexcept;
        [[nodiscard]] bool is_running() const noexcept;
        [[nodiscard]] bool is_normal() const noexcept;
        [[nodiscard]] bool is_dead() const noexcept;
        [[nodiscard]] bool is_resumable() const noexcept;

        // Coroutine execution
        [[nodiscard]] Result<std::vector<runtime::Value>> resume();
        [[nodiscard]] Result<std::vector<runtime::Value>>
        resume(const std::vector<runtime::Value>& args);

        template <typename... Args>
        [[nodiscard]] Result<std::vector<runtime::Value>> resume(Args&&... args);

        [[nodiscard]] Result<std::vector<runtime::Value>> yield();
        [[nodiscard]] Result<std::vector<runtime::Value>>
        yield(const std::vector<runtime::Value>& values);

        template <typename... Args>
        [[nodiscard]] Result<std::vector<runtime::Value>> yield(Args&&... args);

        // Stack management
        void push(const runtime::Value& value);
        [[nodiscard]] runtime::Value pop();
        [[nodiscard]] runtime::Value top() const;
        [[nodiscard]] Size stack_size() const noexcept;
        [[nodiscard]] bool stack_empty() const noexcept;

        template <typename T>
        void push_value(T&& value);

        template <typename T>
        [[nodiscard]] Result<T> pop_as();

        template <typename T>
        [[nodiscard]] Result<T> top_as() const;

        // Error handling
        [[nodiscard]] bool has_error() const noexcept;
        [[nodiscard]] String get_error() const;
        void set_error(const String& error);
        void clear_error();

        // Conversion and access
        [[nodiscard]] runtime::Value to_value() const;
        [[nodiscard]] runtime::GCPtr<runtime::Coroutine> get_coroutine() const;
        [[nodiscard]] const runtime::Coroutine& operator*() const;
        [[nodiscard]] const runtime::Coroutine* operator->() const;

        // Comparison
        bool operator==(const Coroutine& other) const;
        bool operator!=(const Coroutine& other) const;

        // Convenience operators
        template <typename... Args>
        [[nodiscard]] Result<std::vector<runtime::Value>> operator()(Args&&... args);

    private:
        runtime::GCPtr<runtime::Coroutine> coroutine_;

        void ensure_valid() const;
        [[nodiscard]] std::vector<runtime::Value> convert_args() const;

        template <typename T>
        [[nodiscard]] std::vector<runtime::Value> convert_args(T&& arg) const;

        template <typename T, typename... Rest>
        [[nodiscard]] std::vector<runtime::Value> convert_args(T&& arg, Rest&&... rest) const;
    };

    /**
     * @brief Coroutine factory functions
     */
    namespace coroutine_factory {

        /**
         * @brief Create new coroutine with default stack size
         */
        Coroutine create();

        /**
         * @brief Create new coroutine with specified stack size
         */
        Coroutine create(Size stack_size);

        /**
         * @brief Create coroutine from function
         */
        Coroutine from_function(const runtime::Value& function);

    }  // namespace coroutine_factory

}  // namespace rangelua::api
