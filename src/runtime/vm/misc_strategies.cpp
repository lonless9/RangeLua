/**
 * @file misc_strategies.cpp
 * @brief Implementation of miscellaneous instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/misc_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // NotStrategy implementation
    Status NotStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        const Value& operand = context.stack_at(b);

        VM_LOG_DEBUG("NOT: R[{}] := not R[{}]", a, b);

        // Lua NOT: nil and false are falsy, everything else is truthy
        bool result = operand.is_falsy();
        context.stack_at(a) = Value(result);
        return std::monostate{};
    }

    // LenStrategy implementation
    Status LenStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        const Value& operand = context.stack_at(b);

        VM_LOG_DEBUG("LEN: R[{}] := #R[{}]", a, b);

        Value result = operand.length();

        if (result.is_nil()) {
            VM_LOG_ERROR("Invalid length operation on {}", operand.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // Placeholder implementations for other miscellaneous strategies
    Status ConcatStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                        [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("CONCAT: Not fully implemented");
        return std::monostate{};
    }

    Status VarargStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                        [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("VARARG: Not fully implemented");
        return std::monostate{};
    }

    Status VarargPrepStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                            [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("VARARGPREP: Not fully implemented");
        return std::monostate{};
    }

    Status MmbinStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                       [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("MMBIN: Not fully implemented");
        return std::monostate{};
    }

    Status MmbiniStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                        [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("MMBINI: Not fully implemented");
        return std::monostate{};
    }

    Status MmbinkStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                        [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("MMBINK: Not fully implemented");
        return std::monostate{};
    }

    Status ExtraArgStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                          [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("EXTRAARG: Not fully implemented");
        return std::monostate{};
    }

    // MiscStrategyFactory implementation
    void MiscStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering miscellaneous operation strategies");

        registry.register_strategy(std::make_unique<NotStrategy>());
        registry.register_strategy(std::make_unique<LenStrategy>());
        registry.register_strategy(std::make_unique<ConcatStrategy>());
        registry.register_strategy(std::make_unique<VarargStrategy>());
        registry.register_strategy(std::make_unique<VarargPrepStrategy>());
        registry.register_strategy(std::make_unique<MmbinStrategy>());
        registry.register_strategy(std::make_unique<MmbiniStrategy>());
        registry.register_strategy(std::make_unique<MmbinkStrategy>());
        registry.register_strategy(std::make_unique<ExtraArgStrategy>());

        VM_LOG_DEBUG("Registered {} miscellaneous operation strategies", 9);
    }

}  // namespace rangelua::runtime
