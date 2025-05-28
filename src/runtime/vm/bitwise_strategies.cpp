/**
 * @file bitwise_strategies.cpp
 * @brief Implementation of bitwise operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/bitwise_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // Helper function for bitwise operations
    namespace {
        Status perform_bitwise_operation(IVMContext& context,
                                         Instruction instruction,
                                         const char* op_name,
                                         Value (*operation)(const Value&, const Value&)) {
            Register a = backend::InstructionEncoder::decode_a(instruction);
            Register b = backend::InstructionEncoder::decode_b(instruction);
            Register c = backend::InstructionEncoder::decode_c(instruction);

            const Value& left = context.stack_at(b);
            const Value& right = context.stack_at(c);

            VM_LOG_DEBUG("{}: R[{}] := R[{}] {} R[{}]", op_name, a, b, op_name, c);

            try {
                Value result = operation(left, right);

                if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                    VM_LOG_ERROR("Invalid bitwise operation: cannot {} {} and {}",
                                 op_name,
                                 left.type_name(),
                                 right.type_name());
                    return ErrorCode::TYPE_ERROR;
                }

                context.stack_at(a) = std::move(result);
                return std::monostate{};
            } catch (const Exception& e) {
                return e.code();
            } catch (...) {
                return ErrorCode::RUNTIME_ERROR;
            }
        }
    }  // namespace

    // BandStrategy implementation
    Status BandStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_bitwise_operation(
            context, instruction, "BAND", [](const Value& a, const Value& b) { return a & b; });
    }

    Status BorStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_bitwise_operation(
            context, instruction, "BOR", [](const Value& a, const Value& b) { return a | b; });
    }

    Status BxorStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_bitwise_operation(
            context, instruction, "BXOR", [](const Value& a, const Value& b) {
                // TODO: Implement proper XOR in Value class
                return a | b;  // Placeholder
            });
    }

    Status ShlStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_bitwise_operation(
            context, instruction, "SHL", [](const Value& a, const Value& b) { return a << b; });
    }

    Status ShrStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_bitwise_operation(
            context, instruction, "SHR", [](const Value& a, const Value& b) { return a >> b; });
    }

    Status BnotStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        const Value& operand = context.stack_at(b);

        VM_LOG_DEBUG("BNOT: R[{}] := ~R[{}]", a, b);

        try {
            Value result = ~operand;  // Bitwise NOT

            if (result.is_nil() && !operand.is_nil()) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot NOT {}", operand.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            context.stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status BandKStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                       [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("BANDK: Not fully implemented");
        return std::monostate{};
    }

    Status BorKStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                      [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("BORK: Not fully implemented");
        return std::monostate{};
    }

    Status BxorKStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                       [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("BXORK: Not fully implemented");
        return std::monostate{};
    }

    Status ShriStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                      [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("SHRI: Not fully implemented");
        return std::monostate{};
    }

    Status ShliStrategy::execute_impl([[maybe_unused]] IVMContext& context,
                                      [[maybe_unused]] Instruction instruction) {
        VM_LOG_DEBUG("SHLI: Not fully implemented");
        return std::monostate{};
    }

    // BitwiseStrategyFactory implementation
    void BitwiseStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering bitwise operation strategies");

        registry.register_strategy(std::make_unique<BandStrategy>());
        registry.register_strategy(std::make_unique<BorStrategy>());
        registry.register_strategy(std::make_unique<BxorStrategy>());
        registry.register_strategy(std::make_unique<ShlStrategy>());
        registry.register_strategy(std::make_unique<ShrStrategy>());
        registry.register_strategy(std::make_unique<BnotStrategy>());
        registry.register_strategy(std::make_unique<BandKStrategy>());
        registry.register_strategy(std::make_unique<BorKStrategy>());
        registry.register_strategy(std::make_unique<BxorKStrategy>());
        registry.register_strategy(std::make_unique<ShriStrategy>());
        registry.register_strategy(std::make_unique<ShliStrategy>());

        VM_LOG_DEBUG("Registered {} bitwise operation strategies", 11);
    }

}  // namespace rangelua::runtime
