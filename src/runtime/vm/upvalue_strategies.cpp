/**
 * @file upvalue_strategies.cpp
 * @brief Implementation of upvalue operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/upvalue_strategies.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // GetUpvalStrategy implementation
    Status GetUpvalStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        VM_LOG_DEBUG("GETUPVAL: R[{}] := UpValue[{}]", a, b);

        Value value = context.get_upvalue(static_cast<UpvalueIndex>(b));
        context.stack_at(a) = std::move(value);
        return std::monostate{};
    }

    // SetUpvalStrategy implementation
    Status SetUpvalStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        Register b = backend::InstructionEncoder::decode_b(instruction);

        VM_LOG_DEBUG("SETUPVAL: UpValue[{}] := R[{}]", b, a);

        const Value& value = context.stack_at(a);
        context.set_upvalue(static_cast<UpvalueIndex>(b), value);
        return std::monostate{};
    }

    // ClosureStrategy implementation
    Status ClosureStrategy::execute_impl(IVMContext& context, Instruction instruction) {
        Register a = backend::InstructionEncoder::decode_a(instruction);
        std::uint32_t bx = backend::InstructionEncoder::decode_bx(instruction);

        VM_LOG_DEBUG("CLOSURE: R[{}] := closure(KPROTO[{}]) (not fully implemented)", a, bx);

        // For now, just create a simple function value
        // In a full implementation, this would create a closure with upvalues
        context.stack_at(a) = Value{};  // Placeholder
        return std::monostate{};
    }

    // UpvalueStrategyFactory implementation
    void UpvalueStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering upvalue operation strategies");

        registry.register_strategy(std::make_unique<GetUpvalStrategy>());
        registry.register_strategy(std::make_unique<SetUpvalStrategy>());
        registry.register_strategy(std::make_unique<ClosureStrategy>());

        VM_LOG_DEBUG("Registered {} upvalue operation strategies", 3);
    }

}  // namespace rangelua::runtime
