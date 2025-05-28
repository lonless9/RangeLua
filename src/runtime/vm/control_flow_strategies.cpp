/**
 * @file control_flow_strategies.cpp
 * @brief Implementation of control flow instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/control_flow_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // JmpStrategy implementation
    Status JmpStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        std::int32_t sbx = backend::InstructionEncoder::decode_sbx(instruction);

        VM_LOG_DEBUG("JMP: pc += {}", sbx);

        context.adjust_instruction_pointer(sbx);
        return std::monostate{};
    }

    // CallStrategy implementation
    Status CallStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& function = context.stack_at(a);

        VM_LOG_DEBUG("CALL: R[{}], ... ,R[{}] := R[{}](R[{}], ... ,R[{}])",
                     a, a + c - 2, a, a + 1, a + b - 1);

        if (!function.is_function()) {
            VM_LOG_ERROR("Attempt to call a {} value", function.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        // Prepare arguments
        std::vector<Value> args;
        Size arg_count = (b == 0) ? (context.stack_size() - a - 1) : (b - 1);

        args.reserve(arg_count);
        for (Size i = 0; i < arg_count; ++i) {
            args.push_back(context.stack_at(a + 1 + i));
        }

        // Call the function
        std::vector<Value> results;
        auto call_result = context.call_function(function, args, results);
        if (is_error(call_result)) {
            return call_result;
        }

        Size result_count = (c == 0) ? results.size() : (c - 1);

        // Store results
        for (Size i = 0; i < result_count && i < results.size(); ++i) {
            context.stack_at(a + i) = std::move(results[i]);
        }

        // Fill remaining slots with nil if needed
        for (Size i = results.size(); i < result_count; ++i) {
            context.stack_at(a + i) = Value{};
        }

        return std::monostate{};
    }

    // ReturnStrategy implementation
    Status ReturnStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        VM_LOG_DEBUG("RETURN: return R[{}], ... ,R[{}]", a, a + b - 2);

        Size return_count = (b == 0) ? (context.stack_size() - a) : (b - 1);
        return context.return_from_function(return_count);
    }

    // TailCallStrategy implementation
    Status TailCallStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& function = context.stack_at(a);

        VM_LOG_DEBUG("TAILCALL: return R[{}](R[{}], ... ,R[{}])", a, a + 1, a + b - 1);

        if (!function.is_function()) {
            VM_LOG_ERROR("Attempt to call a {} value", function.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        // Prepare arguments
        std::vector<Value> args;
        Size arg_count = (b == 0) ? (context.stack_size() - a - 1) : (b - 1);

        args.reserve(arg_count);
        for (Size i = 0; i < arg_count; ++i) {
            args.push_back(context.stack_at(a + 1 + i));
        }

        // For tail call, we replace the current call frame
        // This is a simplified implementation - proper tail call optimization
        // would reuse the current stack frame
        std::vector<Value> results;
        auto call_result = context.call_function(function, args, results);
        if (is_error(call_result)) {
            return call_result;
        }

        // Return all results from the tail call
        Size result_count = (c == 0) ? results.size() : (c - 1);

        // Move results to the beginning of the current frame
        for (Size i = 0; i < result_count && i < results.size(); ++i) {
            context.stack_at(a + i) = std::move(results[i]);
        }

        // Fill remaining slots with nil if needed
        for (Size i = results.size(); i < result_count; ++i) {
            context.stack_at(a + i) = Value{};
        }

        // Return from current function with the tail call results
        return context.return_from_function(result_count);
    }

    Status Return0Strategy::execute_impl(IVMContext& context,
                                         [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("RETURN0: return");
        return context.return_from_function(0);
    }

    Status Return1Strategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        VM_LOG_DEBUG("RETURN1: return R[{}]", a);
        return context.return_from_function(1);
    }

    // ForLoopStrategy implementation - numeric for loop
    Status ForLoopStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::uint32_t bx = backend::InstructionEncoder::decode_bx(instruction);

        VM_LOG_DEBUG("FORLOOP: update counters; if loop continues then pc-={}", bx);

        // R[A] = index, R[A+1] = limit, R[A+2] = step, R[A+3] = loop variable
        Value& index = context.stack_at(a);
        const Value& limit = context.stack_at(a + 1);
        const Value& step = context.stack_at(a + 2);

        // Update index: index = index + step
        Value new_index = index + step;
        if (new_index.is_nil()) {
            VM_LOG_ERROR("Invalid for loop: cannot add index and step");
            return ErrorCode::TYPE_ERROR;
        }

        index = new_index;

        // Check if loop should continue
        bool continue_loop = false;
        if (step.is_number() && index.is_number() && limit.is_number()) {
            auto step_num = step.to_number();
            auto index_num = index.to_number();
            auto limit_num = limit.to_number();

            if (is_success(step_num) && is_success(index_num) && is_success(limit_num)) {
                Number step_val = get_value(step_num);
                Number index_val = get_value(index_num);
                Number limit_val = get_value(limit_num);

                if (step_val > 0) {
                    continue_loop = (index_val <= limit_val);
                } else {
                    continue_loop = (index_val >= limit_val);
                }
            }
        }

        if (continue_loop) {
            // Copy index to loop variable and jump back
            context.stack_at(a + 3) = index;
            context.adjust_instruction_pointer(-static_cast<std::int32_t>(bx));
        }

        return std::monostate{};
    }

    // ForPrepStrategy implementation - prepare numeric for loop
    Status ForPrepStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::uint32_t bx = backend::InstructionEncoder::decode_bx(instruction);

        VM_LOG_DEBUG("FORPREP: check values and prepare counters; if not to run then pc+={}",
                     bx + 1);

        // R[A] = initial value, R[A+1] = limit, R[A+2] = step
        Value& initial = context.stack_at(a);
        const Value& limit = context.stack_at(a + 1);
        const Value& step = context.stack_at(a + 2);

        // Convert to numbers if needed
        if (!initial.is_number() || !limit.is_number() || !step.is_number()) {
            VM_LOG_ERROR("Invalid for loop: initial, limit, and step must be numbers");
            return ErrorCode::TYPE_ERROR;
        }

        // Subtract step from initial value (will be added back in first FORLOOP)
        Value adjusted_initial = initial - step;
        if (adjusted_initial.is_nil()) {
            VM_LOG_ERROR("Invalid for loop: cannot subtract step from initial value");
            return ErrorCode::TYPE_ERROR;
        }

        initial = adjusted_initial;
        return std::monostate{};
    }

    // TForPrepStrategy implementation - prepare generic for loop
    Status TForPrepStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::uint32_t bx = backend::InstructionEncoder::decode_bx(instruction);

        VM_LOG_DEBUG("TFORPREP: prepare iterator; if not to run then pc+={}", bx + 1);

        // R[A] = iterator function, R[A+1] = state, R[A+2] = control variable
        const Value& iterator = context.stack_at(a);
        [[maybe_unused]] const Value& state = context.stack_at(a + 1);
        [[maybe_unused]] const Value& control = context.stack_at(a + 2);

        // Check if iterator is callable
        if (!iterator.is_function()) {
            VM_LOG_DEBUG("TFORPREP: Iterator is not a function, skipping loop");
            context.adjust_instruction_pointer(static_cast<std::int32_t>(bx + 1));
        }

        // Iterator setup is complete, continue to TFORCALL
        return std::monostate{};
    }

    // TForCallStrategy implementation - call iterator function
    Status TForCallStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        VM_LOG_DEBUG("TFORCALL: R[{}], ... ,R[{}] := R[{}](R[{}], R[{}])",
                     a + 4, a + 3 + c, a, a + 1, a + 2);

        // R[A] = iterator function, R[A+1] = state, R[A+2] = control variable
        const Value& iterator = context.stack_at(a);
        const Value& state = context.stack_at(a + 1);
        const Value& control = context.stack_at(a + 2);

        if (!iterator.is_function()) {
            VM_LOG_ERROR("Attempt to call a {} value in generic for loop", iterator.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        // Prepare arguments: state and control variable
        std::vector<Value> args = {state, control};
        std::vector<Value> results;

        auto call_result = context.call_function(iterator, args, results);
        if (is_error(call_result)) {
            return call_result;
        }

        // Store results in R[A+4], R[A+5], ..., R[A+3+C]
        Size result_count = std::min(static_cast<Size>(c), results.size());
        for (Size i = 0; i < result_count; ++i) {
            context.stack_at(a + 4 + i) = std::move(results[i]);
        }

        // Fill remaining slots with nil
        for (Size i = result_count; i < c; ++i) {
            context.stack_at(a + 4 + i) = Value{};
        }

        // Update control variable with first result (if any)
        if (!results.empty() && !results[0].is_nil()) {
            context.stack_at(a + 2) = results[0];
        }

        return std::monostate{};
    }

    // TForLoopStrategy implementation - check loop continuation
    Status TForLoopStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::uint32_t bx = backend::InstructionEncoder::decode_bx(instruction);

        VM_LOG_DEBUG("TFORLOOP: if R[{}] ~= nil then {{ R[{}] := R[{}]; pc -= {} }}",
                     a + 4, a + 2, a + 4, bx);

        // Check if the first iterator result is not nil
        const Value& first_result = context.stack_at(a + 4);

        if (!first_result.is_nil()) {
            // Continue loop: update control variable and jump back
            context.stack_at(a + 2) = first_result;
            context.adjust_instruction_pointer(-static_cast<std::int32_t>(bx));
        }
        // If nil, loop ends and execution continues

        return std::monostate{};
    }

    // CloseStrategy implementation - close upvalues
    Status CloseStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("CLOSE: close all upvalues >= R[{}]", a);

        // Close all upvalues that refer to stack positions >= R[A]
        // This is typically called when leaving a scope that has local variables
        // that are captured by closures

        // For now, this is a simplified implementation
        // A full implementation would need to:
        // 1. Find all open upvalues that point to stack[A] or higher
        // 2. Close them by copying their values and marking them as closed
        // 3. Update the upvalue chain

        // Since we don't have a full upvalue management system yet,
        // we'll just log the operation
        VM_LOG_DEBUG("CLOSE: Closing upvalues from register {} onwards", a);

        return std::monostate{};
    }

    // TbcStrategy implementation - mark variable as "to be closed"
    Status TbcStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("TBC: mark variable R[{}] as 'to be closed'", a);

        // Mark the variable at R[A] as "to be closed"
        // This is used for Lua 5.4+ to-be-closed variables (local <close> x)
        // When the variable goes out of scope, its __close metamethod is called

        const Value& value = context.stack_at(a);

        // For now, this is a simplified implementation
        // A full implementation would:
        // 1. Check if the value has a __close metamethod
        // 2. Mark it in a to-be-closed list
        // 3. Call the metamethod when the variable goes out of scope

        VM_LOG_DEBUG("TBC: Variable at R[{}] ({}) marked as to-be-closed",
                     a, value.type_name());

        return std::monostate{};
    }

    // ControlFlowStrategyFactory implementation
    void ControlFlowStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering control flow operation strategies");

        registry.register_strategy(std::make_unique<JmpStrategy>());
        registry.register_strategy(std::make_unique<CallStrategy>());
        registry.register_strategy(std::make_unique<TailCallStrategy>());
        registry.register_strategy(std::make_unique<ReturnStrategy>());
        registry.register_strategy(std::make_unique<Return0Strategy>());
        registry.register_strategy(std::make_unique<Return1Strategy>());
        registry.register_strategy(std::make_unique<ForLoopStrategy>());
        registry.register_strategy(std::make_unique<ForPrepStrategy>());
        registry.register_strategy(std::make_unique<TForPrepStrategy>());
        registry.register_strategy(std::make_unique<TForCallStrategy>());
        registry.register_strategy(std::make_unique<TForLoopStrategy>());
        registry.register_strategy(std::make_unique<CloseStrategy>());
        registry.register_strategy(std::make_unique<TbcStrategy>());

        VM_LOG_DEBUG("Registered {} control flow operation strategies", 13);
    }

}  // namespace rangelua::runtime
