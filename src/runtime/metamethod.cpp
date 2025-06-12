/**
 * @file metamethod.cpp
 * @brief Implementation of Lua metamethod system
 * @version 0.1.0
 */

#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/vm/instruction_strategy.hpp>
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

    Result<std::vector<Value>> MetamethodSystem::call_metamethod(IVMContext* context,
                                                               const Value& metamethod,
                                                               const std::vector<Value>& args) {
        VM_LOG_DEBUG("call_metamethod: metamethod type = {}, is_function = {}",
                     metamethod.type_name(),
                     metamethod.is_function());

        if (!metamethod.is_function()) {
            VM_LOG_ERROR("call_metamethod: metamethod is not a function");
            return ErrorCode::TYPE_ERROR;
        }

        VM_LOG_DEBUG("call_metamethod: calling function with {} arguments", args.size());

        // Get the function pointer to check if it's a C function or Lua function
        auto function_result = metamethod.to_function();
        if (is_error(function_result)) {
            VM_LOG_ERROR("call_metamethod: failed to extract function from value");
            return get_error(function_result);
        }

        auto function_ptr = get_value(function_result);

        // Handle C functions directly
        if (function_ptr->isCFunction()) {
            VM_LOG_DEBUG("call_metamethod: calling C function directly");
            try {
                auto result = function_ptr->call(context, args);
                return result;
            } catch (const std::exception& e) {
                VM_LOG_ERROR("call_metamethod: C function call failed: {}", e.what());
                return ErrorCode::RUNTIME_ERROR;
            }
        }

        // For Lua functions, we need to use the VM
        // For now, return an error since we don't have VM context here
        VM_LOG_ERROR("call_metamethod: Lua function calls not yet supported in metamethod system");
        return ErrorCode::RUNTIME_ERROR;
    }

    Result<std::vector<Value>> MetamethodSystem::call_metamethod(IVMContext& context,
                                                                 const Value& metamethod,
                                                                 const std::vector<Value>& args) {
        VM_LOG_DEBUG("call_metamethod (with context): metamethod type = {}, is_function = {}",
                     metamethod.type_name(),
                     metamethod.is_function());

        if (!metamethod.is_function()) {
            VM_LOG_ERROR("call_metamethod: metamethod is not a function");
            return ErrorCode::TYPE_ERROR;
        }

        VM_LOG_DEBUG("call_metamethod: calling function with {} arguments", args.size());

        // Use VM context to call the function (handles both C and Lua functions)
        std::vector<Value> results;
        auto call_status = context.call_function(metamethod, args, results);

        if (std::holds_alternative<ErrorCode>(call_status)) {
            VM_LOG_ERROR("call_metamethod: function call failed");
            return std::get<ErrorCode>(call_status);
        }

        VM_LOG_DEBUG("call_metamethod: function call succeeded with {} results", results.size());
        return results;
    }

    Result<Value> MetamethodSystem::try_binary_metamethod(const Value& left, const Value& right, Metamethod mm) {
        // Try left operand's metatable first
        Value metamethod = get_metamethod(left, mm);

        VM_LOG_DEBUG("try_binary_metamethod: left operand type = {}, metamethod = {}",
                     left.type_name(),
                     metamethod.is_nil() ? "nil" : "found");

        // If not found, try right operand's metatable
        if (metamethod.is_nil()) {
            metamethod = get_metamethod(right, mm);
            VM_LOG_DEBUG("try_binary_metamethod: right operand type = {}, metamethod = {}",
                         right.type_name(),
                         metamethod.is_nil() ? "nil" : "found");
        }

        if (metamethod.is_nil()) {
            VM_LOG_DEBUG("try_binary_metamethod: No metamethod found for {}", get_name(mm));
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        VM_LOG_DEBUG("try_binary_metamethod: Calling metamethod {}", get_name(mm));
        auto call_result = call_metamethod(nullptr, metamethod, {left, right});
        if (is_error(call_result)) {
            VM_LOG_ERROR("try_binary_metamethod: Metamethod call failed");
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            VM_LOG_DEBUG("try_binary_metamethod: Metamethod returned no results");
            return Value{};  // nil
        }

        VM_LOG_DEBUG("try_binary_metamethod: Metamethod returned {} results", results.size());
        return results[0];
    }

    Result<Value> MetamethodSystem::try_binary_metamethod(IVMContext& context, const Value& left, const Value& right, Metamethod mm) {
        // Try left operand's metatable first
        Value metamethod = get_metamethod(left, mm);

        VM_LOG_DEBUG("try_binary_metamethod (with context): left operand type = {}, metamethod = {}",
                     left.type_name(), metamethod.is_nil() ? "nil" : "found");

        // If not found, try right operand's metatable
        if (metamethod.is_nil()) {
            metamethod = get_metamethod(right, mm);
            VM_LOG_DEBUG("try_binary_metamethod (with context): right operand type = {}, metamethod = {}",
                         right.type_name(), metamethod.is_nil() ? "nil" : "found");
        }

        if (metamethod.is_nil()) {
            VM_LOG_DEBUG("try_binary_metamethod (with context): No metamethod found for {}", get_name(mm));
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        VM_LOG_DEBUG("try_binary_metamethod (with context): Calling metamethod {}", get_name(mm));
        auto call_result = call_metamethod(context, metamethod, {left, right});
        if (is_error(call_result)) {
            VM_LOG_ERROR("try_binary_metamethod (with context): Metamethod call failed");
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            VM_LOG_DEBUG("try_binary_metamethod (with context): Metamethod returned no results");
            return Value{};  // nil
        }

        VM_LOG_DEBUG("try_binary_metamethod (with context): Metamethod returned {} results", results.size());
        return results[0];
    }

    Result<Value> MetamethodSystem::try_unary_metamethod(const Value& operand, Metamethod mm) {
        Value metamethod = get_metamethod(operand, mm);

        if (metamethod.is_nil()) {
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        auto call_result = call_metamethod(nullptr, metamethod, {operand});
        if (is_error(call_result)) {
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            return Value{};  // nil
        }

        return results[0];
    }

    Result<Value> MetamethodSystem::try_unary_metamethod(IVMContext& context, const Value& operand, Metamethod mm) {
        Value metamethod = get_metamethod(operand, mm);

        if (metamethod.is_nil()) {
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        auto call_result = call_metamethod(context, metamethod, {operand});
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

        auto call_result = call_metamethod(nullptr, metamethod, {left, right});
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

    Result<bool> MetamethodSystem::try_comparison_metamethod(IVMContext& context, const Value& left, const Value& right, Metamethod mm) {
        // For comparison metamethods, only try left operand's metatable
        // (following Lua 5.5 semantics)
        Value metamethod = get_metamethod(left, mm);

        if (metamethod.is_nil()) {
            return ErrorCode::TYPE_ERROR;  // No metamethod found
        }

        auto call_result = call_metamethod(context, metamethod, {left, right});
        if (is_error(call_result)) {
            return get_error(call_result);
        }

        auto results = get_value(call_result);
        if (results.empty()) {
            return false;  // nil is falsy
        }

        // Convert result to boolean (Lua truthiness)
        return !results[0].is_falsy();
    }

}  // namespace rangelua::runtime
