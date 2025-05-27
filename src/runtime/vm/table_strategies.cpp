/**
 * @file table_strategies.cpp
 * @brief Implementation of table operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/table_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
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
            VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        Value result = table.get(key);
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
            VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        table.set(key, value);
        return std::monostate{};
    }

    // GetTabUpStrategy implementation
    Status GetTabUpStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("GETTABUP: R[{}] := UpValue[{}][R[{}]]", a, b, c);

        // For now, implement as global variable access
        Value constant = context.get_constant(c);
        if (constant.is_string()) {
            auto string_result = constant.to_string();
            if (is_success(string_result)) {
                String name = get_value(string_result);
                Value value = context.get_global(name);
                context.stack_at(a) = std::move(value);
            } else {
                context.stack_at(a) = Value{};
            }
        } else {
            context.stack_at(a) = Value{};
        }

        return std::monostate{};
    }

    // Placeholder implementations for other table strategies
    Status SetTabUpStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("SETTABUP: Not fully implemented");
        return std::monostate{};
    }

    Status GetIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("GETI: Not fully implemented");
        return std::monostate{};
    }

    Status SetIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("SETI: Not fully implemented");
        return std::monostate{};
    }

    Status GetFieldStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("GETFIELD: Not fully implemented");
        return std::monostate{};
    }

    Status SetFieldStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("SETFIELD: Not fully implemented");
        return std::monostate{};
    }

    Status SelfStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("SELF: Not fully implemented");
        return std::monostate{};
    }

    Status SetListStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("SETLIST: Not fully implemented");
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
