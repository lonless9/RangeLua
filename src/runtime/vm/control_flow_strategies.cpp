/**
 * @file control_flow_strategies.cpp
 * @brief Implementation of control flow instruction strategies
 * @version 0.1.0
 */

#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/vm.hpp>
#include <rangelua/runtime/vm/control_flow_strategies.hpp>
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
            // Try __call metamethod for non-function values
            auto metamethod_result =
                MetamethodSystem::try_unary_metamethod(function, Metamethod::CALL);
            if (is_error(metamethod_result)) {
                VM_LOG_ERROR("Attempt to call a {} value", function.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            // If __call metamethod exists, use it as the function
            Value call_metamethod = get_value(metamethod_result);
            if (!call_metamethod.is_function()) {
                VM_LOG_ERROR("__call metamethod is not a function");
                return ErrorCode::TYPE_ERROR;
            }

            // Prepare arguments with the original value as the first argument
            std::vector<Value> args;
            args.push_back(function);  // Add self as first argument

            Size arg_count = (b == 0) ? context.stack_size() - a - 1 : b - 1;
            for (Size i = 0; i < arg_count; ++i) {
                args.push_back(context.stack_at(a + 1 + i));
            }

            // Call the metamethod
            std::vector<Value> results;
            auto call_status = context.call_function(call_metamethod, args, results);
            if (std::holds_alternative<ErrorCode>(call_status)) {
                VM_LOG_ERROR("__call metamethod call failed");
                return std::get<ErrorCode>(call_status);
            }

            // Store results
            Size result_count = (c == 0) ? results.size() : c - 1;
            VM_LOG_DEBUG("Storing {} results from __call metamethod", result_count);

            for (Size i = 0; i < result_count && i < results.size(); ++i) {
                context.stack_at(a + i) = std::move(results[i]);
            }

            // Fill remaining result slots with nil if needed
            for (Size i = results.size(); i < result_count; ++i) {
                context.stack_at(a + i) = Value{};
            }

            return std::monostate{};
        }

        // Prepare arguments
        std::vector<Value> args;
        Size arg_count = (b == 0) ? context.stack_size() - a - 1 : b - 1;

        VM_LOG_DEBUG("Preparing {} arguments for function call", arg_count);
        for (Size i = 0; i < arg_count; ++i) {
            args.push_back(context.stack_at(a + 1 + i));
        }

        // Call the function through the VM context
        std::vector<Value> results;
        auto call_status = context.call_function(function, args, results);
        if (std::holds_alternative<ErrorCode>(call_status)) {
            VM_LOG_ERROR("Function call failed");
            return std::get<ErrorCode>(call_status);
        }

        // Store results
        Size result_count = (c == 0) ? results.size() : c - 1;
        VM_LOG_DEBUG("Storing {} results from function call", result_count);

        for (Size i = 0; i < result_count && i < results.size(); ++i) {
            context.stack_at(a + i) = std::move(results[i]);
        }

        // Fill remaining result slots with nil if needed
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

        // Cast to VirtualMachine to access the new return_from_function method
        if (auto* vm = dynamic_cast<VirtualMachine*>(&context)) {
            return vm->return_from_function(a, return_count);
        } else {
            // Fallback to old method
            return context.return_from_function(return_count);
        }
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
        std::int32_t sbx = backend::InstructionEncoder::decode_sbx(instruction);

        VM_LOG_DEBUG("FORLOOP: update counters; if loop continues then pc-={}", -sbx);

        // R[A] = index, R[A+1] = limit, R[A+2] = step, R[A+3] = loop variable
        Value& index = context.stack_at(a);
        const Value& limit = context.stack_at(a + 1);
        const Value& step = context.stack_at(a + 2);

        VM_LOG_DEBUG("FORLOOP: Before update - index={}, limit={}, step={}",
                     index.debug_string(),
                     limit.debug_string(),
                     step.debug_string());

        // Update index: index = index + step
        Value new_index = index + step;
        if (new_index.is_nil()) {
            VM_LOG_ERROR("Invalid for loop: cannot add index and step");
            return ErrorCode::TYPE_ERROR;
        }

        index = new_index;
        VM_LOG_DEBUG("FORLOOP: After update - index={}", index.debug_string());

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

                VM_LOG_DEBUG("FORLOOP: step_val={}, index_val={}, limit_val={}, continue={}",
                             step_val,
                             index_val,
                             limit_val,
                             continue_loop);
            }
        }

        if (continue_loop) {
            // Copy index to loop variable and jump back
            context.stack_at(a + 3) = index;
            VM_LOG_DEBUG("FORLOOP: Set loop variable R[{}] = {}, jumping back by {}",
                         a + 3,
                         index.debug_string(),
                         -sbx);
            context.adjust_instruction_pointer(sbx);
        } else {
            VM_LOG_DEBUG("FORLOOP: Loop finished, continuing to next instruction");
        }

        return std::monostate{};
    }

    // ForPrepStrategy implementation - prepare numeric for loop
    Status ForPrepStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::int32_t sbx = backend::InstructionEncoder::decode_sbx(instruction);

        VM_LOG_DEBUG("FORPREP: check values and prepare counters; if not to run then pc+={}", sbx);

        // R[A] = initial value, R[A+1] = limit, R[A+2] = step
        Value& initial = context.stack_at(a);
        const Value& limit = context.stack_at(a + 1);
        const Value& step = context.stack_at(a + 2);

        VM_LOG_DEBUG("FORPREP: initial={}, limit={}, step={}",
                     initial.debug_string(),
                     limit.debug_string(),
                     step.debug_string());

        // Convert to numbers if needed
        if (!initial.is_number() || !limit.is_number() || !step.is_number()) {
            VM_LOG_ERROR("Invalid for loop: initial, limit, and step must be numbers");
            return ErrorCode::TYPE_ERROR;
        }

        // Check for zero step
        auto step_result = step.to_number();
        if (is_error(step_result)) {
            VM_LOG_ERROR("Invalid for loop: step is not a valid number");
            return ErrorCode::TYPE_ERROR;
        }
        Number step_val = get_value(step_result);
        if (step_val == 0.0) {
            VM_LOG_ERROR("Invalid for loop: step is zero");
            return ErrorCode::TYPE_ERROR;
        }

        // Check if loop should run at all
        auto initial_result = initial.to_number();
        auto limit_result = limit.to_number();
        if (is_error(initial_result) || is_error(limit_result)) {
            VM_LOG_ERROR("Invalid for loop: initial or limit is not a valid number");
            return ErrorCode::TYPE_ERROR;
        }

        Number initial_val = get_value(initial_result);
        Number limit_val = get_value(limit_result);

        // Check if loop should be skipped
        bool skip_loop = false;
        if (step_val > 0) {
            skip_loop = (initial_val > limit_val);
        } else {
            skip_loop = (initial_val < limit_val);
        }

        if (skip_loop) {
            VM_LOG_DEBUG("FORPREP: Loop condition not met, skipping to pc+{}", sbx);
            context.adjust_instruction_pointer(sbx);
            return std::monostate{};
        }

        // In Lua 5.5, FORPREP doesn't adjust the initial value
        // Instead, it sets up the loop variable and jumps to FORLOOP
        // The FORLOOP instruction handles the increment and test

        // Set the loop variable to the initial value for the first iteration
        context.stack_at(a + 3) = Value(initial_val);
        VM_LOG_DEBUG("FORPREP: Set loop variable R[{}] = {}", a + 3, initial_val);

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

        // Handle iterator function results
        if (results.empty() || (results.size() == 1 && results[0].is_nil())) {
            // Empty result vector or single nil result signals end of iteration
            // Set all result registers to nil to signal loop termination
            VM_LOG_DEBUG("TFORCALL: Iterator returned nil, setting all result registers to nil");
            for (Size i = 0; i < c; ++i) {
                context.stack_at(a + 4 + i) = Value{};
            }

            // Find the corresponding TFORLOOP instruction and jump to it
            // We need to scan forward to find the TFORLOOP instruction with the same base register
            const auto* function = context.current_function();
            if (function) {
                Size current_ip = context.instruction_pointer();
                Size target_ip = current_ip;

                // Scan forward to find TFORLOOP instruction
                for (Size ip = current_ip; ip < function->instructions.size(); ++ip) {
                    Instruction instr = function->instructions[ip];
                    OpCode opcode = backend::InstructionEncoder::decode_opcode(instr);

                    if (opcode == OpCode::OP_TFORLOOP) {
                        Register tfor_a = backend::InstructionEncoder::decode_a(instr);
                        if (tfor_a == a) {
                            // Found the matching TFORLOOP instruction
                            target_ip = ip;
                            break;
                        }
                    }
                }

                if (target_ip != current_ip) {
                    // Jump to the TFORLOOP instruction
                    std::int32_t offset = static_cast<std::int32_t>(target_ip) - static_cast<std::int32_t>(current_ip);
                    VM_LOG_DEBUG("TFORCALL: Jumping to TFORLOOP at offset {}", offset);
                    context.adjust_instruction_pointer(offset);
                } else {
                    VM_LOG_DEBUG("TFORCALL: Could not find matching TFORLOOP, continuing normally");
                }
            }
        } else {
            // Store results in R[A+4], R[A+5], ..., R[A+3+C]
            Size result_count = std::min(static_cast<Size>(c), results.size());
            for (Size i = 0; i < result_count; ++i) {
                context.stack_at(a + 4 + i) = std::move(results[i]);
            }

            // Fill remaining slots with nil only if we have some results
            for (Size i = result_count; i < c; ++i) {
                context.stack_at(a + 4 + i) = Value{};
            }

            // Update control variable with first result (if any)
            if (!results[0].is_nil()) {
                context.stack_at(a + 2) = results[0];
            }
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
        VM_LOG_DEBUG("TFORLOOP: first_result = {}, is_nil = {}",
                     first_result.debug_string(),
                     first_result.is_nil());

        if (!first_result.is_nil()) {
            // Continue loop: update control variable and jump back
            VM_LOG_DEBUG("TFORLOOP: Continuing loop, jumping back");
            context.stack_at(a + 2) = first_result;
            context.adjust_instruction_pointer(-static_cast<std::int32_t>(bx));
        } else {
            VM_LOG_DEBUG("TFORLOOP: Loop terminated, first result is nil");
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
