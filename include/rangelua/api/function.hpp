#pragma once

/**
 * @file function.hpp
 * @brief Comprehensive Function API for RangeLua
 * @version 0.1.0
 *
 * Provides a high-level C++ interface for Lua functions with RAII semantics,
 * type safety, and integration with the runtime objects system.
 */

#include <functional>
#include <vector>

#include "../core/types.hpp"
#include "../runtime/objects.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

    /**
     * @brief High-level Function wrapper with comprehensive API
     *
     * Provides a safe, convenient interface for working with Lua functions.
     * Automatically manages the underlying runtime::Function object and provides
     * type-safe operations with proper error handling.
     */
    class Function {
    public:
        // Function type enumeration
        enum class Type : std::uint8_t {
            C_FUNCTION,    // C function
            LUA_FUNCTION,  // Lua bytecode function
            CLOSURE        // Function with upvalues
        };

        // Construction and conversion
        explicit Function(runtime::Value value);
        explicit Function(runtime::GCPtr<runtime::Function> function);
        explicit Function(
            std::function<std::vector<runtime::Value>(const std::vector<runtime::Value>&)> fn);

        // Copy and move semantics
        Function(const Function& other) = default;
        Function& operator=(const Function& other) = default;
        Function(Function&& other) noexcept = default;
        Function& operator=(Function&& other) noexcept = default;

        ~Function() = default;

        // Validation
        [[nodiscard]] bool is_valid() const noexcept;
        [[nodiscard]] bool is_function() const noexcept;

        // Function properties
        [[nodiscard]] Type type() const;
        [[nodiscard]] Size parameter_count() const noexcept;
        [[nodiscard]] Size upvalue_count() const noexcept;
        [[nodiscard]] bool is_c_function() const noexcept;
        [[nodiscard]] bool is_lua_function() const noexcept;
        [[nodiscard]] bool is_closure() const noexcept;

        // Function calling
        [[nodiscard]] Result<std::vector<runtime::Value>> call() const;
        [[nodiscard]] Result<std::vector<runtime::Value>>
        call(const std::vector<runtime::Value>& args) const;

        template <typename... Args>
        [[nodiscard]] Result<std::vector<runtime::Value>> call(Args&&... args) const;

        // Convenience call methods with type conversion
        template <typename R = runtime::Value>
        [[nodiscard]] Result<R> call_single() const;

        template <typename R = runtime::Value, typename... Args>
        [[nodiscard]] Result<R> call_single(Args&&... args) const;

        // Upvalue management (for closures)
        [[nodiscard]] runtime::Value get_upvalue(Size index) const;
        void set_upvalue(Size index, const runtime::Value& value);
        void add_upvalue(const runtime::Value& value);

        // Bytecode access (for Lua functions)
        [[nodiscard]] std::vector<Instruction> get_bytecode() const;
        [[nodiscard]] std::vector<runtime::Value> get_constants() const;

        // Debug information
        [[nodiscard]] String get_name() const;
        [[nodiscard]] String get_source() const;
        [[nodiscard]] Size get_line_number() const;

        // Conversion and access
        [[nodiscard]] runtime::Value to_value() const;
        [[nodiscard]] runtime::GCPtr<runtime::Function> get_function() const;
        [[nodiscard]] const runtime::Function& operator*() const;
        [[nodiscard]] const runtime::Function* operator->() const;

        // Comparison
        bool operator==(const Function& other) const;
        bool operator!=(const Function& other) const;

        // Function call operator
        [[nodiscard]] Result<std::vector<runtime::Value>>
        operator()(const std::vector<runtime::Value>& args) const;

        template <typename... Args>
        [[nodiscard]] Result<std::vector<runtime::Value>> operator()(Args&&... args) const;

    private:
        runtime::GCPtr<runtime::Function> function_;

        void ensure_valid() const;
        [[nodiscard]] std::vector<runtime::Value> convert_args() const;

        template <typename T>
        [[nodiscard]] std::vector<runtime::Value> convert_args(T&& arg) const;

        template <typename T, typename... Rest>
        [[nodiscard]] std::vector<runtime::Value> convert_args(T&& arg, Rest&&... rest) const;
    };

    /**
     * @brief Function factory functions
     */
    namespace function_factory {

        /**
         * @brief Create function from C++ callable
         */
        template <typename Callable>
        Function from_callable(Callable&& callable);

        /**
         * @brief Create function from C function pointer
         */
        Function from_c_function(
            std::function<std::vector<runtime::Value>(const std::vector<runtime::Value>&)> fn);

        /**
         * @brief Create function from lambda with automatic argument conversion
         */
        template <typename Lambda>
        Function from_lambda(Lambda&& lambda);

    }  // namespace function_factory

}  // namespace rangelua::api
