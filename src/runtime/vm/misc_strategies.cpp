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

    // ConcatStrategy implementation - string concatenation
    Status ConcatStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        VM_LOG_DEBUG("CONCAT: R[{}] := R[{}].. ... ..R[{}]", a, a, a + b - 1);

        // Concatenate values from R[A] to R[A+B-1]
        Value result = context.stack_at(a);

        for (Register i = 1; i < b; ++i) {
            const Value& next = context.stack_at(a + i);
            result = result.concat(next);

            if (result.is_nil()) {
                VM_LOG_ERROR("Invalid concatenation operation");
                return ErrorCode::TYPE_ERROR;
            }
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // VarargStrategy implementation - access vararg parameters
    Status VarargStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("VARARG: R[{}], R[{}], ..., R[{}] = vararg", a, a + 1, a + c - 2);

        // Get vararg values from the current function call
        // For now, this is a simplified implementation
        // A full implementation would access the actual vararg parameters
        // passed to the current function

        Size vararg_count = (c == 0) ? 0 : (c - 1);  // C-1 values to copy

        // Fill registers with nil (no varargs available in this simplified version)
        for (Size i = 0; i < vararg_count; ++i) {
            context.stack_at(a + i) = Value{};  // nil
        }

        VM_LOG_DEBUG("VARARG: Filled {} registers with nil (no varargs available)", vararg_count);
        return std::monostate{};
    }

    // VarargPrepStrategy implementation - prepare vararg parameters
    Status VarargPrepStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("VARARGPREP: adjust vararg parameters at R[{}]", a);

        // Prepare vararg parameters for a function that accepts varargs
        // This instruction adjusts the stack to prepare for vararg access

        // For now, this is a simplified implementation
        // A full implementation would:
        // 1. Calculate how many extra arguments were passed
        // 2. Adjust the stack frame to accommodate vararg access
        // 3. Set up the vararg information for VARARG instructions

        VM_LOG_DEBUG("VARARGPREP: Vararg preparation completed at R[{}]", a);
        return std::monostate{};
    }

    // MmbinStrategy implementation - metamethod binary operation
    Status MmbinStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("MMBIN: R[{}] := metamethod(R[{}], R[{}])", a, b, c);

        [[maybe_unused]] const Value& left = context.stack_at(b);
        [[maybe_unused]] const Value& right = context.stack_at(c);

        // For now, this is a simplified implementation
        // A full implementation would:
        // 1. Determine which metamethod to call based on the operation
        // 2. Look up the metamethod in the operands' metatables
        // 3. Call the metamethod with the operands as arguments
        // 4. Store the result

        // Since we don't have full metamethod support yet, return nil
        context.stack_at(a) = Value{};

        VM_LOG_DEBUG("MMBIN: Metamethod operation not fully implemented, returning nil");
        return std::monostate{};
    }

    // MmbiniStrategy implementation - metamethod binary operation with immediate
    Status MmbiniStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        // C contains the immediate value (signed)
        std::int32_t sc = static_cast<std::int32_t>(c) - 128;

        VM_LOG_DEBUG("MMBINI: R[{}] := metamethod(R[{}], {})", a, b, sc);

        [[maybe_unused]] const Value& left = context.stack_at(b);
        [[maybe_unused]] Value right(static_cast<Number>(sc));

        // For now, this is a simplified implementation
        // A full implementation would call the appropriate metamethod
        context.stack_at(a) = Value{};

        VM_LOG_DEBUG(
            "MMBINI: Metamethod operation with immediate not fully implemented, returning nil");
        return std::monostate{};
    }

    // MmbinkStrategy implementation - metamethod binary operation with constant
    Status MmbinkStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("MMBINK: R[{}] := metamethod(R[{}], K[{}])", a, b, c);

        [[maybe_unused]] const Value& left = context.stack_at(b);
        [[maybe_unused]] const Value& right = context.get_constant(c);

        // For now, this is a simplified implementation
        // A full implementation would call the appropriate metamethod
        context.stack_at(a) = Value{};

        VM_LOG_DEBUG(
            "MMBINK: Metamethod operation with constant not fully implemented, returning nil");
        return std::monostate{};
    }

    // ExtraArgStrategy implementation - extra argument for previous opcode
    Status ExtraArgStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                          Instruction instruction) {
        std::uint32_t ax = backend::InstructionEncoder::decode_ax(instruction);

        VM_LOG_DEBUG("EXTRAARG: extra argument {}", ax);

        // EXTRAARG provides an extra (larger) argument for the previous opcode
        // This instruction doesn't execute by itself - it's consumed by the
        // previous instruction that needs a larger argument than can fit in
        // the normal instruction format

        // The VM should have already processed this instruction when executing
        // the previous opcode, so if we reach here, it's likely an error
        // or the previous instruction didn't consume the EXTRAARG

        VM_LOG_DEBUG(
            "EXTRAARG: Extra argument {} (should have been consumed by previous instruction)", ax);
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
