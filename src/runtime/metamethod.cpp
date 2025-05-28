/**
 * @file metamethod.cpp
 * @brief Implementation of Lua metamethod system
 * @version 0.1.0
 */

#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    std::optional<Metamethod> MetamethodSystem::find_by_name(std::string_view name) noexcept {
        for (size_t i = 0; i < METAMETHOD_NAMES.size(); ++i) {
            if (METAMETHOD_NAMES[i] == name) {
                return static_cast<Metamethod>(i);
            }
        }
        return std::nullopt;
    }

    Value MetamethodSystem::get_metamethod(const Table* table, Metamethod mm) {
        if (!table) {
            return Value{};  // nil
        }

        auto metatable = table->metatable();
        if (!metatable) {
            return Value{};  // nil
        }

        // Create key for metamethod lookup
        std::string_view mm_name = get_name(mm);
        Value key{String(mm_name)};

        return metatable->get(key);
    }

    Value MetamethodSystem::get_metamethod(const Value& value, Metamethod mm) {
        if (value.is_table()) {
            const auto& table_ptr = value.as_table();
            return get_metamethod(table_ptr.get(), mm);
        } else if (value.is_userdata()) {
            const auto& userdata_ptr = value.as_userdata();
            if (userdata_ptr) {
                auto metatable = userdata_ptr->metatable();
                if (metatable) {
                    std::string_view mm_name = get_name(mm);
                    Value key{String(mm_name)};
                    return metatable->get(key);
                }
            }
        }

        return Value{};  // nil
    }

    bool MetamethodSystem::has_metamethod(const Table* table, Metamethod mm) {
        Value metamethod = get_metamethod(table, mm);
        return !metamethod.is_nil();
    }

    bool MetamethodSystem::has_metamethod(const Value& value, Metamethod mm) {
        Value metamethod = get_metamethod(value, mm);
        return !metamethod.is_nil();
    }

    Result<std::vector<Value>> MetamethodSystem::call_metamethod(const Value& metamethod,
                                                               const std::vector<Value>& args) {
        if (!metamethod.is_function()) {
            return ErrorCode::TYPE_ERROR;
        }

        try {
            return metamethod.call(args);
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Result<Value> MetamethodSystem::try_binary_metamethod(const Value& left, const Value& right, Metamethod mm) {
        // Try left operand's metatable first
        Value metamethod = get_metamethod(left, mm);

        // If not found, try right operand's metatable
        if (metamethod.is_nil()) {
            metamethod = get_metamethod(right, mm);
        }

        if (metamethod.is_nil()) {
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        auto call_result = call_metamethod(metamethod, {left, right});
        if (is_error(call_result)) {
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            return Value{};  // nil
        }

        return results[0];
    }

    Result<Value> MetamethodSystem::try_unary_metamethod(const Value& operand, Metamethod mm) {
        Value metamethod = get_metamethod(operand, mm);

        if (metamethod.is_nil()) {
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        auto call_result = call_metamethod(metamethod, {operand});
        if (is_error(call_result)) {
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            return Value{};  // nil
        }

        return results[0];
    }

    Result<bool> MetamethodSystem::try_comparison_metamethod(const Value& left, const Value& right, Metamethod mm) {
        // For comparison metamethods, only try left operand's metatable
        // (following Lua 5.5 semantics)
        Value metamethod = get_metamethod(left, mm);

        if (metamethod.is_nil()) {
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        auto call_result = call_metamethod(metamethod, {left, right});
        if (is_error(call_result)) {
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            return false;
        }

        // Convert result to boolean (Lua truthiness)
        return !results[0].is_falsy();
    }

}  // namespace rangelua::runtime
