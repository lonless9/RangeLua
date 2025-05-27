#pragma once

/**
 * @file bitwise_strategies.hpp
 * @brief Bitwise operation instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_BAND instruction
     * R[A] := R[B] & R[C]
     */
    class BandStrategy : public InstructionStrategyBase<OpCode::OP_BAND> {
    public:
        const char* name() const noexcept override { return "BAND"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_BOR instruction
     * R[A] := R[B] | R[C]
     */
    class BorStrategy : public InstructionStrategyBase<OpCode::OP_BOR> {
    public:
        const char* name() const noexcept override { return "BOR"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_BXOR instruction
     * R[A] := R[B] ~ R[C]
     */
    class BxorStrategy : public InstructionStrategyBase<OpCode::OP_BXOR> {
    public:
        const char* name() const noexcept override { return "BXOR"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SHL instruction
     * R[A] := R[B] << R[C]
     */
    class ShlStrategy : public InstructionStrategyBase<OpCode::OP_SHL> {
    public:
        const char* name() const noexcept override { return "SHL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SHR instruction
     * R[A] := R[B] >> R[C]
     */
    class ShrStrategy : public InstructionStrategyBase<OpCode::OP_SHR> {
    public:
        const char* name() const noexcept override { return "SHR"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_BNOT instruction
     * R[A] := ~R[B]
     */
    class BnotStrategy : public InstructionStrategyBase<OpCode::OP_BNOT> {
    public:
        const char* name() const noexcept override { return "BNOT"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_BANDK instruction
     * R[A] := R[B] & K[C]
     */
    class BandKStrategy : public InstructionStrategyBase<OpCode::OP_BANDK> {
    public:
        const char* name() const noexcept override { return "BANDK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_BORK instruction
     * R[A] := R[B] | K[C]
     */
    class BorKStrategy : public InstructionStrategyBase<OpCode::OP_BORK> {
    public:
        const char* name() const noexcept override { return "BORK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_BXORK instruction
     * R[A] := R[B] ~ K[C]
     */
    class BxorKStrategy : public InstructionStrategyBase<OpCode::OP_BXORK> {
    public:
        const char* name() const noexcept override { return "BXORK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SHRI instruction
     * R[A] := R[B] >> sC
     */
    class ShriStrategy : public InstructionStrategyBase<OpCode::OP_SHRI> {
    public:
        const char* name() const noexcept override { return "SHRI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SHLI instruction
     * R[A] := R[B] << sC
     */
    class ShliStrategy : public InstructionStrategyBase<OpCode::OP_SHLI> {
    public:
        const char* name() const noexcept override { return "SHLI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating bitwise operation strategies
     */
    class BitwiseStrategyFactory {
    public:
        /**
         * @brief Create all bitwise operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        BitwiseStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
