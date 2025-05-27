#pragma once

/**
 * @file upvalue_strategies.hpp
 * @brief Upvalue operation instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_GETUPVAL instruction
     * R[A] := UpValue[B]
     */
    class GetUpvalStrategy : public InstructionStrategyBase<OpCode::OP_GETUPVAL> {
    public:
        const char* name() const noexcept override { return "GETUPVAL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SETUPVAL instruction
     * UpValue[B] := R[A]
     */
    class SetUpvalStrategy : public InstructionStrategyBase<OpCode::OP_SETUPVAL> {
    public:
        const char* name() const noexcept override { return "SETUPVAL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_CLOSURE instruction
     * R[A] := closure(KPROTO[Bx])
     */
    class ClosureStrategy : public InstructionStrategyBase<OpCode::OP_CLOSURE> {
    public:
        const char* name() const noexcept override { return "CLOSURE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating upvalue operation strategies
     */
    class UpvalueStrategyFactory {
    public:
        /**
         * @brief Create all upvalue operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        UpvalueStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
