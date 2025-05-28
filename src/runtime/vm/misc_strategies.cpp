/**
 * @file misc_strategies.cpp
 * @brief Implementation of miscellaneous instruction strategies
 * @version 0.1.0
 */

#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/vm.hpp>
#include <rangelua/runtime/vm/misc_strategies.hpp>
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

        VM_LOG_DEBUG("VARARG: R[{}] = vararg (c={})", a, c);

        // Get current call frame to access vararg information
        auto* vm = dynamic_cast<VirtualMachine*>(&context);
        if (!vm || vm->call_depth() == 0) {
            VM_LOG_ERROR("VARARG: No call frame available");
            return ErrorCode::RUNTIME_ERROR;
        }

        // Access the current call frame to get vararg information
        const CallFrame* current_frame = vm->current_call_frame();
        if (!current_frame) {
            VM_LOG_ERROR("VARARG: No current call frame");
            return ErrorCode::RUNTIME_ERROR;
        }

        Size vararg_count_requested = (c == 0) ? current_frame->vararg_count() : (c - 1);
        Size vararg_count_available = current_frame->vararg_count();

        VM_LOG_DEBUG("VARARG: Requested {} varargs, {} available",
                     vararg_count_requested,
                     vararg_count_available);

        // Copy vararg values to target registers
        for (Size i = 0; i < vararg_count_requested; ++i) {
            if (i < vararg_count_available) {
                // Copy actual vararg value
                // Varargs are stored after the fixed parameters in the stack
                Size vararg_stack_pos = current_frame->stack_base + current_frame->parameter_count + i;
                context.stack_at(a + i) = context.stack_at(vararg_stack_pos);
                VM_LOG_DEBUG("VARARG: R[{}] = vararg[{}] from stack[{}] = {}",
                             a + i,
                             i,
                             vararg_stack_pos,
                             context.stack_at(a + i).debug_string());
            } else {
                // Fill remaining with nil
                context.stack_at(a + i) = Value{};
                VM_LOG_DEBUG("VARARG: R[{}] = nil (no more varargs)", a + i);
            }
        }

        return std::monostate{};
    }

    // VarargPrepStrategy implementation - prepare vararg parameters
    Status VarargPrepStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("VARARGPREP: adjust vararg parameters at R[{}]", a);

        // Get current call frame to access vararg information
        auto* vm = dynamic_cast<VirtualMachine*>(&context);
        if (!vm || vm->call_depth() == 0) {
            VM_LOG_ERROR("VARARGPREP: No call frame available");
            return ErrorCode::RUNTIME_ERROR;
        }

        const CallFrame* current_frame = vm->current_call_frame();
        if (!current_frame) {
            VM_LOG_ERROR("VARARGPREP: No current call frame");
            return ErrorCode::RUNTIME_ERROR;
        }

        // VARARGPREP adjusts the stack to prepare for vararg access
        // The A operand specifies the number of fixed parameters
        // This instruction is typically emitted at the beginning of vararg functions

        if (current_frame->has_varargs) {
            VM_LOG_DEBUG("VARARGPREP: Function has {} fixed params, {} total args, {} varargs",
                         current_frame->parameter_count,
                         current_frame->argument_count,
                         current_frame->vararg_count());

            // The vararg preparation is already done in setup_call_frame
            // This instruction mainly serves as a marker and for stack adjustment
            VM_LOG_DEBUG("VARARGPREP: Vararg preparation completed - {} varargs available",
                         current_frame->vararg_count());
        } else {
            VM_LOG_DEBUG("VARARGPREP: Function does not accept varargs");
        }

        return std::monostate{};
    }

    // MmbinStrategy implementation - metamethod binary operation
    Status MmbinStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("MMBIN: R[{}] := metamethod(R[{}], R[{}])", a, b, c);

        const Value& left = context.stack_at(b);
        const Value& right = context.stack_at(c);

        // The C register contains the metamethod type
        // This maps to the Metamethod enum values
        auto metamethod = static_cast<Metamethod>(c);

        // Try the binary metamethod
        auto result = MetamethodSystem::try_binary_metamethod(left, right, metamethod);
        if (is_error(result)) {
            VM_LOG_ERROR("MMBIN: Metamethod {} failed", MetamethodSystem::get_name(metamethod));
            return get_error(result);
        }

        context.stack_at(a) = get_value(result);
        VM_LOG_DEBUG("MMBIN: Metamethod {} succeeded", MetamethodSystem::get_name(metamethod));
        return std::monostate{};
    }

    // MmbiniStrategy implementation - metamethod binary operation with immediate
    Status MmbiniStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        // In Lua 5.5 MMBINI format: A B C k
        // A = result register, B = left operand register, C = metamethod type, k = immediate flag
        // The immediate value is encoded in B as a signed value
        auto metamethod = static_cast<Metamethod>(c);

        // B contains the immediate value (signed)
        std::int32_t sb = static_cast<std::int32_t>(b) - 128;

        VM_LOG_DEBUG("MMBINI: R[{}] := metamethod({}, R[{}]) [{}]", a, sb, a, MetamethodSystem::get_name(metamethod));

        const Value& left = context.stack_at(a);
        Value right(static_cast<Number>(sb));

        // Try the binary metamethod with immediate value
        auto result = MetamethodSystem::try_binary_metamethod(left, right, metamethod);
        if (is_error(result)) {
            VM_LOG_ERROR("MMBINI: Metamethod {} with immediate failed", MetamethodSystem::get_name(metamethod));
            return get_error(result);
        }

        context.stack_at(a) = get_value(result);
        VM_LOG_DEBUG("MMBINI: Metamethod {} with immediate succeeded", MetamethodSystem::get_name(metamethod));
        return std::monostate{};
    }

    // MmbinkStrategy implementation - metamethod binary operation with constant
    Status MmbinkStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        // In Lua 5.5 MMBINK format: A B C k
        // A = result register, B = constant index, C = metamethod type, k = constant flag
        auto metamethod = static_cast<Metamethod>(c);

        VM_LOG_DEBUG("MMBINK: R[{}] := metamethod(R[{}], K[{}]) [{}]", a, a, b, MetamethodSystem::get_name(metamethod));

        const Value& left = context.stack_at(a);
        const Value& right = context.get_constant(b);

        // Try the binary metamethod with constant value
        auto result = MetamethodSystem::try_binary_metamethod(left, right, metamethod);
        if (is_error(result)) {
            VM_LOG_ERROR("MMBINK: Metamethod {} with constant failed", MetamethodSystem::get_name(metamethod));
            return get_error(result);
        }

        context.stack_at(a) = get_value(result);
        VM_LOG_DEBUG("MMBINK: Metamethod {} with constant succeeded", MetamethodSystem::get_name(metamethod));
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
