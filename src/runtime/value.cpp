/**
 * @file value.cpp
 * @brief Value implementation
 * @version 0.1.0
 */

#include <rangelua/runtime/value.hpp>

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
        return ErrorCode::TYPE_ERROR;
    }

    Result<Value::String> Value::to_string() const {
        if (is_string()) {
            return std::get<String>(data_);
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

    bool Value::operator==(const Value& other) const noexcept {
        if (type() != other.type()) {
            return false;
        }

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
                    return reinterpret_cast<Size>(value.get());
                }
            },
            data_);
    }

    // Stub implementations for operations that need more complex logic
    Value Value::operator+(const Value& other) const {
        // TODO: Implement arithmetic operations
        return Value{};
    }

    Value Value::operator-(const Value& other) const {
        // TODO: Implement arithmetic operations
        return Value{};
    }

    Value Value::operator*(const Value& other) const {
        // TODO: Implement arithmetic operations
        return Value{};
    }

    Value Value::operator/(const Value& other) const {
        // TODO: Implement arithmetic operations
        return Value{};
    }

    Value Value::operator%(const Value& other) const {
        // TODO: Implement arithmetic operations
        return Value{};
    }

    Value Value::operator^(const Value& other) const {
        // TODO: Implement arithmetic operations
        return Value{};
    }

    Value Value::operator-() const {
        // TODO: Implement unary minus
        return Value{};
    }

    Value Value::operator&(const Value& other) const {
        // TODO: Implement bitwise operations
        return Value{};
    }

    Value Value::operator|(const Value& other) const {
        // TODO: Implement bitwise operations
        return Value{};
    }

    Value Value::operator~() const {
        // TODO: Implement bitwise not
        return Value{};
    }

    Value Value::operator<<(const Value& other) const {
        // TODO: Implement bitwise shift
        return Value{};
    }

    Value Value::operator>>(const Value& other) const {
        // TODO: Implement bitwise shift
        return Value{};
    }

    bool Value::operator<(const Value& other) const {
        // TODO: Implement comparison
        return false;
    }

    bool Value::operator<=(const Value& other) const {
        // TODO: Implement comparison
        return false;
    }

    bool Value::operator>(const Value& other) const {
        // TODO: Implement comparison
        return false;
    }

    bool Value::operator>=(const Value& other) const {
        // TODO: Implement comparison
        return false;
    }

    Value Value::concat(const Value& other) const {
        // TODO: Implement string concatenation
        return Value{};
    }

    Value Value::length() const {
        // TODO: Implement length operator
        return Value{};
    }

    Value Value::get(const Value& key) const {
        // TODO: Implement table access
        return Value{};
    }

    void Value::set(const Value& key, const Value& value) {
        // TODO: Implement table assignment
    }

    namespace value_factory {

        Value table() {
            // TODO: Create table
            return Value{};
        }

        Value table(std::initializer_list<std::pair<Value, Value>> init) {
            // TODO: Create table with initial values
            return Value{};
        }

        Value function(std::function<std::vector<Value>(const std::vector<Value>&)> fn) {
            // TODO: Create function
            return Value{};
        }

        Value userdata(void* ptr, String type_name) {
            // TODO: Create userdata
            return Value{};
        }

        Value thread() {
            // TODO: Create thread
            return Value{};
        }

    }  // namespace value_factory

}  // namespace rangelua::runtime
