/**
 * @file value.cpp
 * @brief Value implementation following Lua 5.5 semantics
 * @version 0.1.0
 */

// clang-format off
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/metamethod.hpp>
// clang-format on

#include <rangelua/core/error.hpp>
#include <rangelua/utils/debug.hpp>

#include <cmath>
#include <limits>
#include <sstream>

namespace rangelua::runtime {

    ValueType Value::type() const noexcept {
        return static_cast<ValueType>(data_.index());
    }

    Result<Value::Boolean> Value::to_boolean() const noexcept {
        if (is_boolean()) {
            return std::get<Boolean>(data_);
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::Number> Value::to_number() const noexcept {
        if (is_number()) {
            return std::get<Number>(data_);
        }
        // Try to coerce string to number (Lua 5.5 semantics)
        if (is_string()) {
            return coerce_to_number(*this);
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::String> Value::to_string() const {
        if (is_string()) {
            return std::get<String>(data_);
        }
        // Lua 5.5 coercion: numbers can be converted to strings
        if (is_number()) {
            std::ostringstream oss;
            oss << std::get<Number>(data_);
            return oss.str();
        }

        // Try __tostring metamethod for other types
        auto metamethod_result =
            MetamethodSystem::try_unary_metamethod(*this, Metamethod::TOSTRING);
        if (!is_error(metamethod_result)) {
            Value result = get_value(metamethod_result);
            if (result.is_string()) {
                return std::get<String>(result.data_);
            }
        }

        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::TablePtr> Value::to_table() const noexcept {
        if (is_table()) {
            return std::get<TablePtr>(data_);
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::FunctionPtr> Value::to_function() const noexcept {
        if (is_function()) {
            return std::get<FunctionPtr>(data_);
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::UserdataPtr> Value::to_userdata() const noexcept {
        if (is_userdata()) {
            return std::get<UserdataPtr>(data_);
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::ThreadPtr> Value::to_thread() const noexcept {
        if (is_thread()) {
            return std::get<ThreadPtr>(data_);
        }
        return ErrorCode::TYPE_ERROR;
    }

    bool Value::is_truthy() const noexcept {
        if (is_nil())
            return false;
        if (is_boolean())
            return std::get<Boolean>(data_);
        return true;  // All other values are truthy in Lua
    }

    GCObject* Value::as_gc_object() const noexcept {
        if (is_table()) {
            return static_cast<GCObject*>(std::get<TablePtr>(data_).get());
        }
        if (is_function()) {
            return static_cast<GCObject*>(std::get<FunctionPtr>(data_).get());
        }
        if (is_userdata()) {
            return static_cast<GCObject*>(std::get<UserdataPtr>(data_).get());
        }
        if (is_thread()) {
            return static_cast<GCObject*>(std::get<ThreadPtr>(data_).get());
        }
        return nullptr;
    }

    bool Value::operator==(const Value& other) const noexcept {
        // Same type comparison
        if (type() == other.type()) {
            return std::visit(
                [&other](const auto& value) {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, Nil>) {
                        return true;  // All nils are equal
                    } else {
                        return value == std::get<T>(other.data_);
                    }
                },
                data_);
        }

        // Cross-type comparison: numbers and numeric strings (Lua 5.5 semantics)
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);
        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            return std::get<Number>(a_num) == std::get<Number>(b_num);
        }

        return false;  // Different types that can't be coerced are not equal
    }

    String Value::type_name() const {
        switch (type()) {
            case ValueType::Nil:
                return "nil";
            case ValueType::Boolean:
                return "boolean";
            case ValueType::Number:
                return "number";
            case ValueType::String:
                return "string";
            case ValueType::Table:
                return "table";
            case ValueType::Function:
                return "function";
            case ValueType::Userdata:
                return "userdata";
            case ValueType::Thread:
                return "thread";
            default:
                return "unknown";
        }
    }

    String Value::debug_string() const {
        std::ostringstream oss;

        switch (type()) {
            case ValueType::Nil:
                oss << "nil";
                break;
            case ValueType::Boolean:
                oss << (std::get<Boolean>(data_) ? "true" : "false");
                break;
            case ValueType::Number:
                oss << std::get<Number>(data_);
                break;
            case ValueType::String:
                oss << "\"" << std::get<String>(data_) << "\"";
                break;
            case ValueType::Table:
                oss << "table: " << std::get<TablePtr>(data_).get();
                break;
            case ValueType::Function:
                oss << "function: " << std::get<FunctionPtr>(data_).get();
                break;
            case ValueType::Userdata:
                oss << "userdata: " << std::get<UserdataPtr>(data_).get();
                break;
            case ValueType::Thread:
                oss << "thread: " << std::get<ThreadPtr>(data_).get();
                break;
        }

        return oss.str();
    }

    Size Value::hash() const noexcept {
        return std::visit(
            [](const auto& value) -> Size {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, Nil>) {
                    return 0;
                } else if constexpr (std::is_same_v<T, Boolean>) {
                    return std::hash<bool>{}(value);
                } else if constexpr (std::is_same_v<T, Number>) {
                    return std::hash<double>{}(value);
                } else if constexpr (std::is_same_v<T, String>) {
                    return std::hash<std::string>{}(value);
                } else {
                    // For GC pointers, use the pointer value as hash
                    return std::hash<void*>{}(value.get());
                }
            },
            data_);
    }

    // Arithmetic operations with Lua 5.5 semantics and enhanced error handling
    Value Value::operator+(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            return Value(std::get<Number>(a_num) + std::get<Number>(b_num));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::ADD);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        // Enhanced error handling for invalid arithmetic operations
        RANGELUA_DEBUG_PRINT("Arithmetic error: Cannot add " +
                             std::to_string(static_cast<int>(type())) + " and " +
                             std::to_string(static_cast<int>(other.type())));

        // Log the error for debugging
        log_error(ErrorCode::TYPE_ERROR, "Invalid arithmetic operation: addition");

        return Value{};
    }

    Value Value::operator-(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            return Value(std::get<Number>(a_num) - std::get<Number>(b_num));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::SUB);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator*(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            return Value(std::get<Number>(a_num) * std::get<Number>(b_num));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::MUL);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator/(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            Number divisor = std::get<Number>(b_num);
            if (divisor != 0.0) {
                return Value(std::get<Number>(a_num) / divisor);
            }
            // Division by zero - in Lua this results in inf or -inf
            Number dividend = std::get<Number>(a_num);
            if (dividend > 0.0) {
                return Value(std::numeric_limits<Number>::infinity());
            }
            if (dividend < 0.0) {
                return Value(-std::numeric_limits<Number>::infinity());
            }
            return Value(std::numeric_limits<Number>::quiet_NaN());
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::DIV);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator%(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            Number divisor = std::get<Number>(b_num);
            if (divisor != 0.0) {
                return Value(std::fmod(std::get<Number>(a_num), divisor));
            }
            return Value(std::numeric_limits<Number>::quiet_NaN());
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::MOD);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator^(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            return Value(std::pow(std::get<Number>(a_num), std::get<Number>(b_num)));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::POW);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator-() const {
        auto num = coerce_to_number(*this);

        if (std::holds_alternative<Number>(num)) {
            return Value(-std::get<Number>(num));
        }

        // Try metamethod fallback
        auto metamethod_result = MetamethodSystem::try_unary_metamethod(*this, Metamethod::UNM);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator&(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            // Convert to unsigned integers for bitwise operations
            auto a_uint = static_cast<UInt>(std::get<Number>(a_num));
            auto b_uint = static_cast<UInt>(std::get<Number>(b_num));
            return Value(static_cast<Number>(a_uint & b_uint));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::BAND);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator|(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            // Convert to unsigned integers for bitwise operations
            auto a_uint = static_cast<UInt>(std::get<Number>(a_num));
            auto b_uint = static_cast<UInt>(std::get<Number>(b_num));
            return Value(static_cast<Number>(a_uint | b_uint));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::BOR);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::bitwise_xor(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            // Convert to unsigned integers for bitwise operations
            auto a_uint = static_cast<UInt>(std::get<Number>(a_num));
            auto b_uint = static_cast<UInt>(std::get<Number>(b_num));
            return Value(static_cast<Number>(a_uint ^ b_uint));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::BXOR);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator~() const {
        auto num = coerce_to_number(*this);

        if (std::holds_alternative<Number>(num)) {
            // Convert to signed integer, perform bitwise NOT, keep as signed
            // Following Lua 5.5 implementation: intop(^, ~l_castS2U(0), v1)
            auto int_val = static_cast<Int>(std::get<Number>(num));
            auto uint_val = static_cast<UInt>(int_val);
            auto result_uint = ~uint_val;
            auto result_int = static_cast<Int>(result_uint);
            return Value(static_cast<Number>(result_int));
        }

        // Try metamethod fallback
        auto metamethod_result = MetamethodSystem::try_unary_metamethod(*this, Metamethod::BNOT);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator<<(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            // Convert to unsigned integers for bitwise operations
            auto a_uint = static_cast<UInt>(std::get<Number>(a_num));
            auto b_uint = static_cast<UInt>(std::get<Number>(b_num));
            return Value(static_cast<Number>(a_uint << b_uint));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::SHL);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    Value Value::operator>>(const Value& other) const {
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);

        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            // Convert to unsigned integers for bitwise operations
            auto a_uint = static_cast<UInt>(std::get<Number>(a_num));
            auto b_uint = static_cast<UInt>(std::get<Number>(b_num));
            return Value(static_cast<Number>(a_uint >> b_uint));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::SHR);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};
    }

    bool Value::operator<(const Value& other) const {
        if (!are_comparable(*this, other)) {
            // Try metamethod fallback for incomparable types
            auto metamethod_result =
                MetamethodSystem::try_comparison_metamethod(*this, other, Metamethod::LT);
            if (!is_error(metamethod_result)) {
                return get_value(metamethod_result);
            }
            return false;  // In Lua, incomparable values are never less than each other
        }

        // Same type comparison
        if (type() == other.type()) {
            switch (type()) {
                case ValueType::Number:
                    return std::get<Number>(data_) < std::get<Number>(other.data_);
                case ValueType::String:
                    return std::get<String>(data_) < std::get<String>(other.data_);
                default:
                    // Try metamethod for other types
                    auto metamethod_result =
                        MetamethodSystem::try_comparison_metamethod(*this, other, Metamethod::LT);
                    if (!is_error(metamethod_result)) {
                        return get_value(metamethod_result);
                    }
                    return false;  // Other types are not ordered
            }
        }

        // Cross-type comparison (numbers and numeric strings)
        auto a_num = coerce_to_number(*this);
        auto b_num = coerce_to_number(other);
        if (std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num)) {
            return std::get<Number>(a_num) < std::get<Number>(b_num);
        }

        return false;
    }

    bool Value::operator<=(const Value& other) const {
        // Try metamethod first
        auto metamethod_result =
            MetamethodSystem::try_comparison_metamethod(*this, other, Metamethod::LE);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        // Fallback to < or ==
        return *this < other || *this == other;
    }

    bool Value::operator>(const Value& other) const {
        return other < *this;
    }

    bool Value::operator>=(const Value& other) const {
        return other < *this || *this == other;
    }

    Value Value::concat(const Value& other) const {
        // In Lua, concatenation coerces numbers to strings
        auto a_str = coerce_to_string(*this);
        auto b_str = coerce_to_string(other);

        if (std::holds_alternative<String>(a_str) && std::holds_alternative<String>(b_str)) {
            return Value(std::get<String>(a_str) + std::get<String>(b_str));
        }

        // Try metamethod fallback
        auto metamethod_result =
            MetamethodSystem::try_binary_metamethod(*this, other, Metamethod::CONCAT);
        if (!is_error(metamethod_result)) {
            return get_value(metamethod_result);
        }

        return Value{};  // Error case - should be handled by VM
    }

    Value Value::length() const {
        switch (type()) {
            case ValueType::String:
                return Value(static_cast<Number>(std::get<String>(data_).length()));
            case ValueType::Table:
                // For tables, return the length of the array part
                if (const auto& table_ptr = std::get<TablePtr>(data_)) {
                    return Value(static_cast<Number>(table_ptr->arraySize()));
                }
                return Value(0.0);
            default:
                // Try metamethod fallback
                auto metamethod_result =
                    MetamethodSystem::try_unary_metamethod(*this, Metamethod::LEN);
                if (!is_error(metamethod_result)) {
                    return get_value(metamethod_result);
                }
                return Value{};  // Error case - should be handled by VM
        }
    }

    Value Value::get(const Value& key) const {
        if (is_table()) {
            const auto& table_ptr = std::get<TablePtr>(data_);
            if (table_ptr) {
                Value result = table_ptr->get(key);
                // If key not found in table, try __index metamethod
                if (result.is_nil()) {
                    auto metamethod =
                        MetamethodSystem::get_metamethod(table_ptr.get(), Metamethod::INDEX);
                    if (!metamethod.is_nil()) {
                        if (metamethod.is_function()) {
                            auto call_result =
                                MetamethodSystem::call_metamethod(nullptr, metamethod, {*this, key});
                            if (!is_error(call_result)) {
                                auto results = get_value(call_result);
                                if (!results.empty()) {
                                    return results[0];
                                }
                            }
                        } else if (metamethod.is_table()) {
                            // If __index is a table, recursively look up the key
                            return metamethod.get(key);
                        }
                    }
                }
                return result;
            }
        }
        return Value{};  // Non-table values return nil
    }

    void Value::set(const Value& key, const Value& value) {
        if (is_table()) {
            const auto& table_ptr = std::get<TablePtr>(data_);
            if (table_ptr) {
                // Check if key already exists in table
                Value existing = table_ptr->get(key);
                if (existing.is_nil()) {
                    // Key doesn't exist, try __newindex metamethod
                    auto metamethod =
                        MetamethodSystem::get_metamethod(table_ptr.get(), Metamethod::NEWINDEX);
                    if (!metamethod.is_nil()) {
                        if (metamethod.is_function()) {
                            MetamethodSystem::call_metamethod(nullptr, metamethod, {*this, key, value});
                            return;
                        } else if (metamethod.is_table()) {
                            // If __newindex is a table, set the key in that table
                            metamethod.set(key, value);
                            return;
                        }
                    }
                }
                // Either key exists or no __newindex metamethod, set normally
                table_ptr->set(key, value);
            }
        }
        // Non-table values ignore assignment (should be handled by VM)
    }

    namespace value_factory {

        Value table() {
            auto table_ptr = makeGCObject<Table>();
            return Value(std::move(table_ptr));
        }

        Value table(std::initializer_list<std::pair<Value, Value>> init) {
            auto table_ptr = makeGCObject<Table>();
            // Initialize the table with the provided key-value pairs
            for (const auto& [key, value] : init) {
                table_ptr->set(key, value);
            }
            // Move the GCPtr into the Value to transfer ownership
            return Value(std::move(table_ptr));
        }

        Value function(
            const std::function<std::vector<Value>(IVMContext*, const std::vector<Value>&)>& fn,
            IVMContext* vm_context) {
            auto function_ptr = makeGCObject<Function>(fn, vm_context);
            return Value(std::move(function_ptr));
        }

        Value closure(const std::vector<Instruction>& bytecode, Size paramCount) {
            auto function_ptr = makeGCObject<Function>(bytecode, paramCount);
            function_ptr->makeClosure();
            return Value(std::move(function_ptr));
        }

        Value closure(const std::vector<Instruction>& bytecode,
                      const std::vector<Value>& upvalues,
                      Size paramCount) {
            auto function_ptr = makeGCObject<Function>(bytecode, paramCount);
            function_ptr->makeClosure();

            // Add upvalues as closed upvalues
            for (const auto& upvalue : upvalues) {
                function_ptr->addUpvalue(upvalue);
            }

            return Value(std::move(function_ptr));
        }

        Value userdata(void* ptr, const String& type_name) {
            // For now, assume the size is unknown - this should be improved
            auto userdata_ptr = makeGCObject<Userdata>(ptr, 0, type_name);
            return Value(std::move(userdata_ptr));
        }

        Value thread() {
            auto coroutine_ptr = makeGCObject<Coroutine>();
            return Value(std::move(coroutine_ptr));
        }

    }  // namespace value_factory

    // Template method implementations
    template <typename... Args>
    Result<std::vector<Value>> Value::call(IVMContext* vm, Args&&... args) const {
        if (!is_function()) {
            return ErrorCode::TYPE_ERROR;
        }

        const auto& function_ptr = std::get<FunctionPtr>(data_);
        if (!function_ptr) {
            return ErrorCode::RUNTIME_ERROR;
        }

        // Convert arguments to vector
        std::vector<Value> arg_vector;
        if constexpr (sizeof...(args) == 1) {
            // Special case for single vector argument
            using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
            if constexpr (std::is_same_v<std::decay_t<FirstArg>, std::vector<Value>>) {
                auto&& first_arg = std::get<0>(std::forward_as_tuple(args...));
                arg_vector = std::forward<decltype(first_arg)>(first_arg);
            } else {
                (arg_vector.emplace_back(std::forward<Args>(args)), ...);
            }
        } else {
            (arg_vector.emplace_back(std::forward<Args>(args)), ...);
        }

        // Call the function
        try {
            // The `call` on `Value` doesn't have a VM context. The context must be
            // baked into the C-function `Value` at creation time.
            auto result = function_ptr->call(vm, arg_vector);
            return result;
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    // Explicit instantiations for common argument patterns
    template Result<std::vector<Value>> Value::call<>(IVMContext*) const;
    template Result<std::vector<Value>> Value::call<const Value&>(IVMContext*, const Value&) const;
    template Result<std::vector<Value>>
    Value::call<const Value&, const Value&>(IVMContext*, const Value&, const Value&) const;
    template Result<std::vector<Value>>
    Value::call<const Value&, const Value&, const Value&>(IVMContext*,
                                                          const Value&,
                                                          const Value&,
                                                          const Value&) const;
    template Result<std::vector<Value>>
    Value::call<const std::vector<Value>&>(IVMContext*, const std::vector<Value>&) const;
    template Result<std::vector<Value>>
    Value::call<std::vector<Value>&>(IVMContext*, std::vector<Value>&) const;

    // Helper methods implementation
    Result<Value::Number> Value::coerce_to_number(const Value& value) noexcept {
        if (value.is_number()) {
            return std::get<Number>(value.data_);
        }
        if (value.is_string()) {
            const auto& str = std::get<String>(value.data_);
            // Try to parse as number (following Lua 5.5 rules)
            char* end = nullptr;
            double result = std::strtod(str.c_str(), &end);
            // Check if entire string was consumed (valid number)
            if (end != nullptr && static_cast<Size>(end - str.c_str()) == str.length()) {
                return result;
            }
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::String> Value::coerce_to_string(const Value& value) {
        if (value.is_string()) {
            return std::get<String>(value.data_);
        }
        if (value.is_number()) {
            std::ostringstream oss;
            oss << std::get<Number>(value.data_);
            return oss.str();
        }
        if (value.is_nil()) {
            return String("nil");
        }
        if (value.is_boolean()) {
            return value.as_boolean() ? String("true") : String("false");
        }
        if (value.is_table()) {
            std::ostringstream oss;
            oss << "table: " << std::get<TablePtr>(value.data_).get();
            return oss.str();
        }
        if (value.is_function()) {
            std::ostringstream oss;
            oss << "function: " << std::get<FunctionPtr>(value.data_).get();
            return oss.str();
        }
        if (value.is_userdata()) {
            std::ostringstream oss;
            oss << "userdata: " << std::get<UserdataPtr>(value.data_).get();
            return oss.str();
        }
        if (value.is_thread()) {
            std::ostringstream oss;
            oss << "thread: " << std::get<ThreadPtr>(value.data_).get();
            return oss.str();
        }
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::String> Value::tostring_with_metamethod(const Value& value) {
        // Handle basic types that don't need metamethods
        if (value.is_nil()) {
            return String("nil");
        }
        if (value.is_boolean()) {
            return value.as_boolean() ? String("true") : String("false");
        }
        if (value.is_string()) {
            return value.as_string();
        }
        if (value.is_number()) {
            Number num = value.as_number();
            // Format number similar to Lua's default formatting
            if (num == static_cast<double>(static_cast<Int>(num))) {
                return std::to_string(static_cast<Int>(num));
            } else {
                std::string result = std::to_string(num);
                // Remove trailing zeros
                result.erase(result.find_last_not_of('0') + 1, std::string::npos);
                result.erase(result.find_last_not_of('.') + 1, std::string::npos);
                return result;
            }
        }

        // For other types (table, function, userdata, thread), try __tostring metamethod
        // First check if there's a metamethod available
        Value metamethod = MetamethodSystem::get_metamethod(value, Metamethod::TOSTRING);
        if (!metamethod.is_nil()) {
            // Try to call the metamethod (this will only work for C functions without VM context)
            auto metamethod_result =
                MetamethodSystem::try_unary_metamethod(value, Metamethod::TOSTRING);
            if (!is_error(metamethod_result)) {
                Value result = get_value(metamethod_result);
                if (result.is_string()) {
                    return result.as_string();
                }
            }
            // If metamethod call failed (likely because it's a Lua function),
            // we can't call it from here without VM context.
            // This is a limitation that should be addressed by calling tostring from VM level.
        }

        // Fallback to default representation
        if (value.is_table()) {
            return String("table");
        }
        return value.debug_string();
    }

    bool Value::are_comparable(const Value& a, const Value& b) noexcept {
        // In Lua, values are comparable if they are the same type
        // or if both can be coerced to numbers
        if (a.type() == b.type()) {
            return true;
        }
        // Numbers and strings that can be converted to numbers are comparable
        auto a_num = coerce_to_number(a);
        auto b_num = coerce_to_number(b);
        return std::holds_alternative<Number>(a_num) && std::holds_alternative<Number>(b_num);
    }

    // Value comparison utilities implementation
    namespace value_comparison {

        bool raw_equal(const Value& a, const Value& b) noexcept {
            // Raw equality - no metamethods, just direct comparison
            return a == b;
        }

        bool lua_equal(const Value& a, const Value& b) noexcept {
            // Lua equality semantics (for now, same as raw_equal)
            // In a full implementation, this would check metamethods
            return raw_equal(a, b);
        }

        std::strong_ordering compare(const Value& a, const Value& b) {
            if (a == b) {
                return std::strong_ordering::equal;
            }
            if (a < b) {
                return std::strong_ordering::less;
            }
            if (a > b) {
                return std::strong_ordering::greater;
            }
            // Values that are not comparable
            return std::strong_ordering::equivalent;
        }

    }  // namespace value_comparison

}  // namespace rangelua::runtime
