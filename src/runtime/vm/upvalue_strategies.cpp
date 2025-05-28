/**
 * @file upvalue_strategies.cpp
 * @brief Implementation of upvalue operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/vm/upvalue_strategies.hpp>
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

        VM_LOG_DEBUG("CLOSURE: R[{}] := closure(KPROTO[{}])", a, bx);

        // Get the current function to access prototypes
        const auto* current_function = context.current_function();
        if (!current_function) {
            VM_LOG_ERROR("CLOSURE: No current function");
            return ErrorCode::RUNTIME_ERROR;
        }

        // Check if prototype index is valid
        if (bx >= current_function->prototypes.size()) {
            VM_LOG_ERROR("CLOSURE: Invalid prototype index {}", bx);
            return ErrorCode::RUNTIME_ERROR;
        }

        const auto& prototype = current_function->prototypes[bx];

        // Create a new function from the prototype
        auto function = makeGCObject<Function>(prototype.instructions, prototype.parameter_count);
        function->makeClosure();  // Mark as closure

        // Copy vararg flag from prototype
        function->setVararg(prototype.is_vararg);

        // Copy constants from prototype to function
        for (const auto& constant : prototype.constants) {
            // Convert ConstantValue to Value
            Value value = std::visit(
                [](const auto& val) -> Value {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        return Value{};
                    } else if constexpr (std::is_same_v<T, bool>) {
                        return Value(val);
                    } else if constexpr (std::is_same_v<T, Number>) {
                        return Value(val);
                    } else if constexpr (std::is_same_v<T, Int>) {
                        return Value(val);
                    } else if constexpr (std::is_same_v<T, String>) {
                        return Value(val);
                    } else {
                        return Value{};
                    }
                },
                constant);
            function->addConstant(value);
        }

        // Create upvalues based on the prototype's upvalue descriptors
        for (const auto& desc : prototype.upvalue_descriptors) {
            GCPtr<Upvalue> upvalue;

            if (desc.in_stack) {
                // Upvalue refers to a local variable in the current function
                Value* stack_location = &context.stack_at(desc.index);
                upvalue = makeGCObject<Upvalue>(stack_location);
                VM_LOG_DEBUG("CLOSURE: Created open upvalue for stack[{}]", desc.index);
            } else {
                // Upvalue refers to an upvalue in the current function
                Value upvalue_value = context.get_upvalue(desc.index);
                upvalue = makeGCObject<Upvalue>(upvalue_value);
                VM_LOG_DEBUG("CLOSURE: Created closed upvalue from upvalue[{}]", desc.index);
            }

            function->addUpvalue(upvalue);
        }

        // If the function has no upvalues, add _ENV as upvalue[0] (Lua 5.5 semantics)
        if (prototype.upvalue_descriptors.empty()) {
            // Get _ENV from the current function's upvalue[0]
            Value env_value = context.get_upvalue(0);
            auto env_upvalue = makeGCObject<Upvalue>(env_value);
            function->addUpvalue(env_upvalue);
            VM_LOG_DEBUG("CLOSURE: Added _ENV as upvalue[0] for function without upvalues");
        }

        // Store the closure in the register
        context.stack_at(a) = Value(function);

        VM_LOG_DEBUG("CLOSURE: Created closure with {} upvalues",
                     prototype.upvalue_descriptors.size());
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
