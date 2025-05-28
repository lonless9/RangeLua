/**
 * @file arithmetic_strategies.cpp
 * @brief Implementation of arithmetic operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/arithmetic_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // Helper function for arithmetic operations
    namespace {
        Status perform_arithmetic_operation(IVMContext& context, Instruction instruction,
                                           const char* op_name,
                                           Value (*operation)(const Value&, const Value&)) {
            Register a = backend::InstructionEncoder::decode_a(instruction);
            Register b = backend::InstructionEncoder::decode_b(instruction);
            Register c = backend::InstructionEncoder::decode_c(instruction);

            const Value& left = context.stack_at(b);
            const Value& right = context.stack_at(c);

            VM_LOG_DEBUG("{}: R[{}] := R[{}] {} R[{}]", op_name, a, b, op_name, c);
            VM_LOG_DEBUG(
                "{}: left value = {} (type: {})", op_name, left.debug_string(), left.type_name());
            VM_LOG_DEBUG("{}: right value = {} (type: {})",
                         op_name,
                         right.debug_string(),
                         right.type_name());

            Value result = operation(left, right);

            VM_LOG_DEBUG(
                "{}: result = {} (type: {})", op_name, result.debug_string(), result.type_name());

            // Check if the operation resulted in nil (error case)
            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot {} {} and {}",
                           op_name, left.type_name(), right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            context.stack_at(a) = std::move(result);
            return std::monostate{};
        }
    }

    // AddStrategy implementation
    Status AddStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_arithmetic_operation(context, instruction, "ADD",
                                          [](const Value& a, const Value& b) { return a + b; });
    }

    // SubStrategy implementation
    Status SubStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_arithmetic_operation(context, instruction, "SUB",
                                          [](const Value& a, const Value& b) { return a - b; });
    }

    // MulStrategy implementation
    Status MulStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_arithmetic_operation(context, instruction, "MUL",
                                          [](const Value& a, const Value& b) { return a * b; });
    }

    // DivStrategy implementation
    Status DivStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_arithmetic_operation(context, instruction, "DIV",
                                          [](const Value& a, const Value& b) { return a / b; });
    }

    // ModStrategy implementation
    Status ModStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_arithmetic_operation(context, instruction, "MOD",
                                          [](const Value& a, const Value& b) { return a % b; });
    }

    // PowStrategy implementation
    Status PowStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        return perform_arithmetic_operation(context, instruction, "POW",
                                          [](const Value& a, const Value& b) { return a ^ b; });
    }

    // IDivStrategy implementation
    Status IDivStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        const Value& right = context.stack_at(c);

        VM_LOG_DEBUG("IDIV: R[{}] := R[{}] // R[{}]", a, b, c);

        // TODO: Implement integer division in Value class
        Value result = left / right;  // Placeholder using regular division

        if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
            VM_LOG_ERROR("Invalid arithmetic operation: cannot integer divide {} and {}",
                       left.type_name(), right.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // UnmStrategy implementation
    Status UnmStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        const Value& operand = context.stack_at(b);

        VM_LOG_DEBUG("UNM: R[{}] := -R[{}]", a, b);

        Value result = -operand;  // Unary minus

        if (result.is_nil() && !operand.is_nil()) {
            VM_LOG_ERROR("Invalid arithmetic operation: cannot negate {}", operand.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // Immediate arithmetic operations
    Status AddIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        std::int8_t c = static_cast<std::int8_t>(backend::InstructionEncoder::decode_c(instruction));

        const Value& left = context.stack_at(b);
        Value right = Value(static_cast<Int>(c));

        VM_LOG_DEBUG("ADDI: R[{}] := R[{}] + {}", a, b, c);

        Value result = left + right;

        if (result.is_nil() && !left.is_nil()) {
            VM_LOG_ERROR("Invalid arithmetic operation: cannot add {} and immediate {}",
                       left.type_name(), c);
            return ErrorCode::TYPE_ERROR;
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // Constant arithmetic operations
    Status AddKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("ADDK: R[{}] := R[{}] + K[{}]", a, b, c);

        Value result = left + right;

        if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
            VM_LOG_ERROR("Invalid arithmetic operation: cannot add {} and constant {}",
                       left.type_name(), right.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // Similar implementations for other constant operations...
    Status SubKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("SUBK: R[{}] := R[{}] - K[{}]", a, b, c);

        Value result = left - right;
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    Status MulKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("MULK: R[{}] := R[{}] * K[{}]", a, b, c);

        Value result = left * right;
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    Status ModKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("MODK: R[{}] := R[{}] % K[{}]", a, b, c);

        Value result = left % right;
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    Status PowKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("POWK: R[{}] := R[{}] ^ K[{}]", a, b, c);

        Value result = left ^ right;
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    Status DivKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("DIVK: R[{}] := R[{}] / K[{}]", a, b, c);

        Value result = left / right;
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    Status IDivKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& left = context.stack_at(b);
        Value right = context.get_constant(c);

        VM_LOG_DEBUG("IDIVK: R[{}] := R[{}] // K[{}]", a, b, c);

        // TODO: Implement integer division
        Value result = left / right;  // Placeholder
        context.stack_at(a) = std::move(result);
        return std::monostate{};
    }

    // ArithmeticStrategyFactory implementation
    void ArithmeticStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering arithmetic operation strategies");

        // Basic arithmetic operations
        registry.register_strategy(std::make_unique<AddStrategy>());
        registry.register_strategy(std::make_unique<SubStrategy>());
        registry.register_strategy(std::make_unique<MulStrategy>());
        registry.register_strategy(std::make_unique<DivStrategy>());
        registry.register_strategy(std::make_unique<ModStrategy>());
        registry.register_strategy(std::make_unique<PowStrategy>());
        registry.register_strategy(std::make_unique<IDivStrategy>());
        registry.register_strategy(std::make_unique<UnmStrategy>());

        // Immediate operations
        registry.register_strategy(std::make_unique<AddIStrategy>());

        // Constant operations
        registry.register_strategy(std::make_unique<AddKStrategy>());
        registry.register_strategy(std::make_unique<SubKStrategy>());
        registry.register_strategy(std::make_unique<MulKStrategy>());
        registry.register_strategy(std::make_unique<ModKStrategy>());
        registry.register_strategy(std::make_unique<PowKStrategy>());
        registry.register_strategy(std::make_unique<DivKStrategy>());
        registry.register_strategy(std::make_unique<IDivKStrategy>());

        VM_LOG_DEBUG("Registered {} arithmetic operation strategies", 15);
    }

}  // namespace rangelua::runtime
