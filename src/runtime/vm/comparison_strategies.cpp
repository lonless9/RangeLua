/**
 * @file comparison_strategies.cpp
 * @brief Implementation of comparison operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/comparison_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // EqStrategy implementation
    Status EqStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        std::int16_t k = static_cast<std::int16_t>(backend::InstructionEncoder::decode_c(instruction));

        const Value& left = context.stack_at(a);
        const Value& right = context.stack_at(b);
        bool result = (left == right);

        VM_LOG_DEBUG("EQ: if ((R[{}] == R[{}]) ~= {}) then pc++", a, b, k);

        // If k is non-zero, we want to jump if the comparison is false
        if ((k != 0) == result) {
            context.adjust_instruction_pointer(1);  // Skip next instruction
        }

        return std::monostate{};
    }

    // LtStrategy implementation
    Status LtStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        std::int16_t k = static_cast<std::int16_t>(backend::InstructionEncoder::decode_c(instruction));

        const Value& left = context.stack_at(a);
        const Value& right = context.stack_at(b);
        bool result = (left < right);

        VM_LOG_DEBUG("LT: if ((R[{}] < R[{}]) ~= {}) then pc++", a, b, k);

        if ((k != 0) == result) {
            context.adjust_instruction_pointer(1);
        }

        return std::monostate{};
    }

    // LeStrategy implementation
    Status LeStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        std::int16_t k = static_cast<std::int16_t>(backend::InstructionEncoder::decode_c(instruction));

        const Value& left = context.stack_at(a);
        const Value& right = context.stack_at(b);
        bool result = (left <= right);

        VM_LOG_DEBUG("LE: if ((R[{}] <= R[{}]) ~= {}) then pc++", a, b, k);

        if ((k != 0) == result) {
            context.adjust_instruction_pointer(1);
        }

        return std::monostate{};
    }

    // TestStrategy implementation
    Status TestStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& value = context.stack_at(a);
        bool is_truthy = value.is_truthy();

        VM_LOG_DEBUG("TEST: if (not R[{}] == {}) then pc++", a, c);

        // If c is non-zero, we want to jump if the value is falsy
        if ((c != 0) != is_truthy) {
            context.adjust_instruction_pointer(1);
        }

        return std::monostate{};
    }

    // TestSetStrategy implementation
    Status TestSetStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);
        Register c = backend::InstructionEncoder::decode_c(instruction);

        const Value& value = context.stack_at(b);
        bool is_truthy = value.is_truthy();

        VM_LOG_DEBUG("TESTSET: if (not R[{}] == {}) then pc++ else R[{}] := R[{}]", b, c, a, b);

        if ((c != 0) != is_truthy) {
            context.adjust_instruction_pointer(1);
        } else {
            context.stack_at(a) = value;
        }

        return std::monostate{};
    }

    // Placeholder implementations for other comparison strategies
    Status EqKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("EQK: Not fully implemented");
        return std::monostate{};
    }

    Status EqIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("EQI: Not fully implemented");
        return std::monostate{};
    }

    Status LtIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("LTI: Not fully implemented");
        return std::monostate{};
    }

    Status LeIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("LEI: Not fully implemented");
        return std::monostate{};
    }

    Status GtIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("GTI: Not fully implemented");
        return std::monostate{};
    }

    Status GeIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        VM_LOG_DEBUG("GEI: Not fully implemented");
        return std::monostate{};
    }

    // ComparisonStrategyFactory implementation
    void ComparisonStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering comparison operation strategies");

        registry.register_strategy(std::make_unique<EqStrategy>());
        registry.register_strategy(std::make_unique<LtStrategy>());
        registry.register_strategy(std::make_unique<LeStrategy>());
        registry.register_strategy(std::make_unique<EqKStrategy>());
        registry.register_strategy(std::make_unique<EqIStrategy>());
        registry.register_strategy(std::make_unique<LtIStrategy>());
        registry.register_strategy(std::make_unique<LeIStrategy>());
        registry.register_strategy(std::make_unique<GtIStrategy>());
        registry.register_strategy(std::make_unique<GeIStrategy>());
        registry.register_strategy(std::make_unique<TestStrategy>());
        registry.register_strategy(std::make_unique<TestSetStrategy>());

        VM_LOG_DEBUG("Registered {} comparison operation strategies", 11);
    }

}  // namespace rangelua::runtime
