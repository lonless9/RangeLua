/**
 * @file table_strategies.cpp
 * @brief Implementation of table operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/vm/table_strategies.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // NewTableStrategy implementation
    Status NewTableStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("NEWTABLE: R[{}] := {{}}", a);

        // Create new table using value factory
        Value table = value_factory::table();
        context.stack_at(a) = std::move(table);
        return std::monostate{};
    }

    // GetTableStrategy implementation
    Status GetTableStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& table = context.stack_at(b);
        const Value& key = context.stack_at(c);

        VM_LOG_DEBUG("GETTABLE: R[{}] := R[{}][R[{}]]", a, b, c);

        if (!table.is_table()) {
            // Try __index metamethod for non-table values
            auto mm_result = MetamethodSystem::try_binary_metamethod(table, key, Metamethod::INDEX);
            if (is_error(mm_result)) {
                VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
                return ErrorCode::TYPE_ERROR;
            }
            context.stack_at(a) = get_value(mm_result);
            return std::monostate{};
        }

        // Get value from table
        Value result = table.get(key);

        // If result is nil and table has __index metamethod, try it
        if (result.is_nil() && MetamethodSystem::has_metamethod(table, Metamethod::INDEX)) {
            auto mm_result = MetamethodSystem::try_binary_metamethod(table, key, Metamethod::INDEX);
            if (!is_error(mm_result)) {
                result = get_value(mm_result);
            }
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // SetTableStrategy implementation
    Status SetTableStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        Value& table = context.stack_at(a);
        const Value& key = context.stack_at(b);
        const Value& value = context.stack_at(c);

        VM_LOG_DEBUG("SETTABLE: R[{}][R[{}]] := R[{}]", a, b, c);

        if (!table.is_table()) {
            // Try __newindex metamethod for non-table values
            auto mm_result =
                MetamethodSystem::try_binary_metamethod(table, key, Metamethod::NEWINDEX);
            if (is_error(mm_result)) {
                VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
                return ErrorCode::TYPE_ERROR;
            }
            return std::monostate{};
        }

        // Check if key exists in table
        Value existing = table.get(key);

        // If key doesn't exist and table has __newindex metamethod, try it
        if (existing.is_nil() && MetamethodSystem::has_metamethod(table, Metamethod::NEWINDEX)) {
            auto mm_result = MetamethodSystem::call_metamethod(
                MetamethodSystem::get_metamethod(table, Metamethod::NEWINDEX), {table, key, value});
            if (!is_error(mm_result)) {
                return std::monostate{};
            }
        }

        // Normal table assignment
        table.set(key, value);
        return std::monostate{};
    }

    // GetTabUpStrategy implementation
    Status GetTabUpStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("GETTABUP: R[{}] := UpValue[{}][K[{}]]", a, b, c);

        // In Lua 5.5, GETTABUP is: R[A] := UpValue[B][K[C]]
        // B = upvalue index (0 for _ENV), C = constant index for key

        // Get the upvalue (should be _ENV for global access)
        Value upvalue = context.get_upvalue(static_cast<UpvalueIndex>(b));
        Value key = context.get_constant(c);

        if (upvalue.is_table()) {
            // Proper table access through _ENV upvalue
            Value result = upvalue.get(key);
            context.stack_at(a) = std::move(result);

            if (key.is_string()) {
                auto string_result = key.to_string();
                if (is_success(string_result)) {
                    String name = get_value(string_result);
                    VM_LOG_DEBUG(
                        "GETTABUP: Loaded from _ENV '{}' = {}", name, result.debug_string());
                }
            }
        } else {
            // Fallback to global variable access for compatibility
            if (key.is_string()) {
                auto string_result = key.to_string();
                if (is_success(string_result)) {
                    String name = get_value(string_result);
                    Value value = context.get_global(name);
                    context.stack_at(a) = std::move(value);
                    VM_LOG_DEBUG("GETTABUP: Fallback global '{}' = {}", name, value.debug_string());
                } else {
                    context.stack_at(a) = Value{};
                }
            } else {
                context.stack_at(a) = Value{};
            }
        }

        return std::monostate{};
    }

    // SetTabUpStrategy implementation
    Status SetTabUpStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("SETTABUP: UpValue[{}][K[{}]] := R[{}]", a, b, c);

        // In Lua 5.5, SETTABUP is: UpValue[A][K[B]] := R[C]
        // A = upvalue index (0 for _ENV), B = constant index for key, C = value register

        // Get the upvalue (should be _ENV for global access)
        Value upvalue = context.get_upvalue(static_cast<UpvalueIndex>(a));
        Value key = context.get_constant(b);
        const Value& value = context.stack_at(c);

        if (upvalue.is_table()) {
            // Proper table assignment through _ENV upvalue
            upvalue.set(key, value);

            if (key.is_string()) {
                auto string_result = key.to_string();
                if (is_success(string_result)) {
                    String name = get_value(string_result);
                    VM_LOG_DEBUG("SETTABUP: Set in _ENV '{}' = {}", name, value.debug_string());
                }
            }
        } else {
            // Fallback to global variable assignment for compatibility
            if (key.is_string()) {
                auto string_result = key.to_string();
                if (is_success(string_result)) {
                    String name = get_value(string_result);
                    context.set_global(name, value);
                    VM_LOG_DEBUG("SETTABUP: Fallback global '{}' = {}", name, value.debug_string());
                }
            }
        }

        return std::monostate{};
    }

    // GetIStrategy implementation - table access with integer index
    Status GetIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& table = context.stack_at(b);
        Value key(static_cast<Number>(c));  // C is the integer index

        VM_LOG_DEBUG("GETI: R[{}] := R[{}][{}]", a, b, c);

        if (!table.is_table()) {
            VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        Value result = table.get(key);
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // SetIStrategy implementation - table assignment with integer index
    Status SetIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        Value& table = context.stack_at(a);
        Value key(static_cast<Number>(b));  // B is the integer index
        const Value& value = context.stack_at(c);

        VM_LOG_DEBUG("SETI: R[{}][{}] := R[{}]", a, b, c);

        if (!table.is_table()) {
            VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        table.set(key, value);
        return std::monostate{};
    }

    // GetFieldStrategy implementation - table access with constant string key
    Status GetFieldStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& table = context.stack_at(b);
        const Value& key = context.get_constant(c);

        VM_LOG_DEBUG("GETFIELD: R[{}] := R[{}][K[{}]]", a, b, c);

        if (!table.is_table()) {
            VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        Value result = table.get(key);
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // SetFieldStrategy implementation - table assignment with constant string key
    Status SetFieldStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        Value& table = context.stack_at(a);
        const Value& key = context.get_constant(b);
        const Value& value = context.stack_at(c);

        VM_LOG_DEBUG("SETFIELD: R[{}][K[{}]] := R[{}]", a, b, c);

        if (!table.is_table()) {
            VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        table.set(key, value);
        return std::monostate{};
    }

    // SelfStrategy implementation - method call preparation
    Status SelfStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& table = context.stack_at(b);
        const Value& key = context.get_constant(c);

        VM_LOG_DEBUG("SELF: R[{}] := R[{}]; R[{}] := R[{}][K[{}]]", a + 1, b, a, b, c);

        // R[A+1] := R[B] (copy table to next register)
        context.stack_at(a + 1) = table;

        // R[A] := R[B][K[C]] (get method from table)
        if (table.is_table()) {
            Value method = table.get(key);
            context.stack_at(a) = std::move(method);
        } else {
            context.stack_at(a) = Value{};
        }

        return std::monostate{};
    }

    // SetListStrategy implementation - set list elements
    Status SetListStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        Value& table = context.stack_at(a);

        VM_LOG_DEBUG("SETLIST: R[{}][{}+i] := R[{}+i], 1 <= i <= {}", a, c, a, b);

        if (!table.is_table()) {
            VM_LOG_ERROR("Attempt to set list elements on a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        // Set list elements: R[A][C+i] := R[A+i] for i = 1 to B
        for (Register i = 1; i <= b; ++i) {
            Value key(static_cast<Number>(c + i));
            const Value& value = context.stack_at(a + i);
            table.set(key, value);
        }

        return std::monostate{};
    }

    // TableStrategyFactory implementation
    void TableStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering table operation strategies");

        registry.register_strategy(std::make_unique<NewTableStrategy>());
        registry.register_strategy(std::make_unique<GetTableStrategy>());
        registry.register_strategy(std::make_unique<SetTableStrategy>());
        registry.register_strategy(std::make_unique<GetTabUpStrategy>());
        registry.register_strategy(std::make_unique<SetTabUpStrategy>());
        registry.register_strategy(std::make_unique<GetIStrategy>());
        registry.register_strategy(std::make_unique<SetIStrategy>());
        registry.register_strategy(std::make_unique<GetFieldStrategy>());
        registry.register_strategy(std::make_unique<SetFieldStrategy>());
        registry.register_strategy(std::make_unique<SelfStrategy>());
        registry.register_strategy(std::make_unique<SetListStrategy>());

        VM_LOG_DEBUG("Registered {} table operation strategies", 11);
    }

}  // namespace rangelua::runtime
