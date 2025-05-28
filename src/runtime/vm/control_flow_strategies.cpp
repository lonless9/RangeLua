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

    // Placeholder implementations for other control flow strategies
    Status TailCallStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                          [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("TAILCALL: Not fully implemented");
        return std::monostate{};
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

    Status TForPrepStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                          [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("TFORPREP: Not fully implemented");
        return std::monostate{};
    }

    Status TForCallStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                          [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("TFORCALL: Not fully implemented");
        return std::monostate{};
    }

    Status TForLoopStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                          [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("TFORLOOP: Not fully implemented");
        return std::monostate{};
    }

    Status CloseStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                       [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("CLOSE: Not fully implemented");
        return std::monostate{};
    }

    Status TbcStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                     [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("TBC: Not fully implemented");
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
