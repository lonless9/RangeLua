#pragma once

/**
 * @file error.hpp
 * @brief Error handling system with modern C++20 features
 * @version 0.1.0
 */

#include <exception>
#include <source_location>
#include <system_error>

#include "types.hpp"

namespace rangelua {

    /**
     * @brief Base exception class for all RangeLua errors
     */
    class Exception : public std::exception {
    public:
        explicit Exception(StringView message,
                           ErrorCode code = ErrorCode::UNKNOWN_ERROR,
                           std::source_location location = std::source_location::current()) noexcept
            : message_(message), code_(code), location_(location) {}

        [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }

        [[nodiscard]] ErrorCode code() const noexcept { return code_; }
        [[nodiscard]] const std::source_location& location() const noexcept { return location_; }

        [[nodiscard]] String detailed_message() const;

    private:
        String message_;
        ErrorCode code_;
        std::source_location location_;
    };

    /**
     * @brief Syntax error during parsing
     */
    class SyntaxError : public Exception {
    public:
        explicit SyntaxError(
            StringView message,
            SourceLocation source_loc = {},
            std::source_location location = std::source_location::current()) noexcept
            : Exception(message, ErrorCode::SYNTAX_ERROR, location),
              source_location_(std::move(source_loc)) {}

        [[nodiscard]] const SourceLocation& source_location() const noexcept {
            return source_location_;
        }

    private:
        SourceLocation source_location_;
    };

    /**
     * @brief Runtime error during execution
     */
    class RuntimeError : public Exception {
    public:
        explicit RuntimeError(
            StringView message,
            std::source_location location = std::source_location::current()) noexcept
            : Exception(message, ErrorCode::RUNTIME_ERROR, location) {}
    };

    /**
     * @brief Memory allocation error
     */
    class MemoryError : public Exception {
    public:
        explicit MemoryError(
            StringView message,
            Size requested_size = 0,
            std::source_location location = std::source_location::current()) noexcept
            : Exception(message, ErrorCode::MEMORY_ERROR, location),
              requested_size_(requested_size) {}

        [[nodiscard]] Size requested_size() const noexcept { return requested_size_; }

    private:
        Size requested_size_;
    };

    /**
     * @brief Type error during operations
     */
    class TypeError : public Exception {
    public:
        explicit TypeError(StringView message,
                           StringView expected_type = "",
                           StringView actual_type = "",
                           std::source_location location = std::source_location::current()) noexcept
            : Exception(message, ErrorCode::TYPE_ERROR, location),
              expected_type_(expected_type),
              actual_type_(actual_type) {}

        [[nodiscard]] StringView expected_type() const noexcept { return expected_type_; }
        [[nodiscard]] StringView actual_type() const noexcept { return actual_type_; }

    private:
        String expected_type_;
        String actual_type_;
    };

    /**
     * @brief Stack overflow error
     */
    class StackOverflowError : public Exception {
    public:
        explicit StackOverflowError(
            Size stack_size = 0,
            std::source_location location = std::source_location::current()) noexcept
            : Exception("Stack overflow", ErrorCode::STACK_OVERFLOW, location),
              stack_size_(stack_size) {}

        [[nodiscard]] Size stack_size() const noexcept { return stack_size_; }

    private:
        Size stack_size_;
    };

    /**
     * @brief Error category for RangeLua errors
     */
    class ErrorCategory : public std::error_category {
    public:
        [[nodiscard]] const char* name() const noexcept override { return "rangelua"; }

        [[nodiscard]] String message(int error_value) const override;

        static const ErrorCategory& instance() {
            static ErrorCategory category;
            return category;
        }
    };

    /**
     * @brief Create error code from ErrorCode enum
     */
    inline std::error_code make_error_code(ErrorCode code) noexcept {
        return {static_cast<int>(code), ErrorCategory::instance()};
    }

    // Note: Result type is already defined in types.hpp as std::variant<T, ErrorCode>
    // We use the existing Result type alias from types.hpp

    // Convenience aliases for void results
    using Status = Result<std::monostate>;

    // Helper functions for error handling
    template <typename T>
    [[nodiscard]] constexpr bool is_success(const Result<T>& result) noexcept {
        return std::holds_alternative<T>(result);
    }

    template <typename T>
    [[nodiscard]] constexpr bool is_error(const Result<T>& result) noexcept {
        return std::holds_alternative<ErrorCode>(result);
    }

    template <typename T>
    [[nodiscard]] constexpr ErrorCode get_error(const Result<T>& result) noexcept {
        return std::get<ErrorCode>(result);
    }

    template <typename T>
    [[nodiscard]] constexpr const T& get_value(const Result<T>& result) {
        return std::get<T>(result);
    }

    template <typename T>
    [[nodiscard]] constexpr T&& get_value(Result<T>&& result) {
        return std::get<T>(std::move(result));
    }

    // Factory functions for creating results (declared early for template dependencies)
    template <typename T>
    [[nodiscard]] constexpr Result<std::decay_t<T>> make_success(T&& value) {
        return Result<std::decay_t<T>>{std::forward<T>(value)};
    }

    [[nodiscard]] constexpr Status make_success() {
        return Status{std::monostate{}};
    }

    template <typename T>
    [[nodiscard]] constexpr Result<T> make_error(ErrorCode error) {
        return Result<T>{error};
    }

    // Modern C++20 error handling with monadic operations
    template <typename T, typename F>
    constexpr auto and_then(const Result<T>& result, F&& func)
        -> decltype(func(std::declval<T>())) {
        if (is_success(result)) {
            return std::forward<F>(func)(get_value(result));
        }
        return get_error(result);
    }

    template <typename T, typename F>
    constexpr auto or_else(const Result<T>& result, F&& func) -> Result<T> {
        if (is_error(result)) {
            return std::forward<F>(func)(get_error(result));
        }
        return result;
    }

    template <typename T, typename F>
    constexpr auto transform(const Result<T>& result, F&& func)
        -> Result<decltype(func(std::declval<T>()))> {
        if (is_success(result)) {
            return make_success(std::forward<F>(func)(get_value(result)));
        }
        return make_error<decltype(func(std::declval<T>()))>(get_error(result));
    }

    template <typename T, typename F>
    constexpr auto transform_error(const Result<T>& result, F&& func) -> Result<T> {
        if (is_error(result)) {
            auto new_error = std::forward<F>(func)(get_error(result));
            return make_error<T>(new_error);
        }
        return result;
    }

    // Error handling utilities
    template <typename T>
    [[nodiscard]] constexpr T value_or(const Result<T>& result, T&& default_value) {
        if (is_success(result)) {
            return get_value(result);
        }
        return std::forward<T>(default_value);
    }

    template <typename T, typename F>
    [[nodiscard]] constexpr T value_or_else(const Result<T>& result, F&& func) {
        if (is_success(result)) {
            return get_value(result);
        }
        return std::forward<F>(func)(get_error(result));
    }

    // Error propagation helper function for cleaner error handling
    template <typename T>
    constexpr auto try_unwrap(Result<T>&& result) -> T {
        if (is_error(result)) {
            throw RuntimeError("TRY operation failed with error: " +
                               String(error_code_to_string(get_error(result))));
        }
        return get_value(std::move(result));
    }

// Error propagation macro for cleaner error handling (C++20 compatible)
#define RANGELUA_TRY(expr) rangelua::try_unwrap(expr)

    // Error logging utilities
    void log_error(const Exception& ex);
    void log_error(ErrorCode code, StringView message);

    // Error formatting utilities
    String format_error_message(ErrorCode code, StringView context = "");
    String format_exception_details(const Exception& ex);
    StringView error_code_to_string(ErrorCode code) noexcept;

}  // namespace rangelua

// Enable std::error_code support
namespace std {
    template <>
    struct is_error_code_enum<rangelua::ErrorCode> : true_type {};
}  // namespace std
