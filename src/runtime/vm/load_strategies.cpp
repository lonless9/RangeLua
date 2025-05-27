/**
 * @file load_strategies.cpp
 * @brief Implementation of load operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/load_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // MoveStrategy implementation
    Status MoveStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        VM_LOG_DEBUG("MOVE: R[{}] := R[{}]", a, b);

        context.stack_at(a) = context.stack_at(b);
        return std::monostate{};
    }

    // LoadIStrategy implementation
    Status LoadIStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::int32_t sbx = backend::InstructionEncoder::decode_sbx(instruction);

        VM_LOG_DEBUG("LOADI: R[{}] := {}", a, sbx);

        context.stack_at(a) = Value(static_cast<Int>(sbx));
        return std::monostate{};
    }

    // LoadFStrategy implementation
    Status LoadFStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::int32_t sbx = backend::InstructionEncoder::decode_sbx(instruction);

        VM_LOG_DEBUG("LOADF: R[{}] := {}", a, static_cast<Number>(sbx));

        context.stack_at(a) = Value(static_cast<Number>(sbx));
        return std::monostate{};
    }

    // LoadKStrategy implementation
    Status LoadKStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::uint32_t bx = backend::InstructionEncoder::decode_bx(instruction);

        VM_LOG_DEBUG("LOADK: R[{}] := K[{}]", a, bx);

        Value constant = context.get_constant(static_cast<std::uint16_t>(bx));
        context.stack_at(a) = std::move(constant);
        return std::monostate{};
    }

    // LoadKXStrategy implementation
    Status LoadKXStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("LOADKX: R[{}] := K[extra arg] (not fully implemented)", a);

        // TODO: Implement LOADKX with extra argument from next instruction
        context.stack_at(a) = Value{};  // Placeholder
        return std::monostate{};
    }

    // LoadFalseStrategy implementation
    Status LoadFalseStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("LOADFALSE: R[{}] := false", a);

        context.stack_at(a) = Value(false);
        return std::monostate{};
    }

    // LFalseSkipStrategy implementation
    Status LFalseSkipStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("LFALSESKIP: R[{}] := false; pc++", a);

        context.stack_at(a) = Value(false);
        context.adjust_instruction_pointer(1);  // Skip next instruction
        return std::monostate{};
    }

    // LoadTrueStrategy implementation
    Status LoadTrueStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);

        VM_LOG_DEBUG("LOADTRUE: R[{}] := true", a);

        context.stack_at(a) = Value(true);
        return std::monostate{};
    }

    // LoadNilStrategy implementation
    Status LoadNilStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        VM_LOG_DEBUG("LOADNIL: R[{}] to R[{}] := nil", a, a + b);

        for (Register i = a; i <= a + b; ++i) {
            context.stack_at(i) = Value{};
        }
        return std::monostate{};
    }

    // LoadStrategyFactory implementation
    void LoadStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering load operation strategies");

        registry.register_strategy(std::make_unique<MoveStrategy>());
        registry.register_strategy(std::make_unique<LoadIStrategy>());
        registry.register_strategy(std::make_unique<LoadFStrategy>());
        registry.register_strategy(std::make_unique<LoadKStrategy>());
        registry.register_strategy(std::make_unique<LoadKXStrategy>());
        registry.register_strategy(std::make_unique<LoadFalseStrategy>());
        registry.register_strategy(std::make_unique<LFalseSkipStrategy>());
        registry.register_strategy(std::make_unique<LoadTrueStrategy>());
        registry.register_strategy(std::make_unique<LoadNilStrategy>());

        VM_LOG_DEBUG("Registered {} load operation strategies", 9);
    }

}  // namespace rangelua::runtime
