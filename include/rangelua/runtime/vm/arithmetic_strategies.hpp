#pragma once

/**
 * @file arithmetic_strategies.hpp
 * @brief Arithmetic operation instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_ADD instruction
     * R[A] := R[B] + R[C]
     */
    class AddStrategy : public InstructionStrategyBase<OpCode::OP_ADD> {
    public:
        const char* name() const noexcept override { return "ADD"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SUB instruction
     * R[A] := R[B] - R[C]
     */
    class SubStrategy : public InstructionStrategyBase<OpCode::OP_SUB> {
    public:
        const char* name() const noexcept override { return "SUB"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MUL instruction
     * R[A] := R[B] * R[C]
     */
    class MulStrategy : public InstructionStrategyBase<OpCode::OP_MUL> {
    public:
        const char* name() const noexcept override { return "MUL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_DIV instruction
     * R[A] := R[B] / R[C]
     */
    class DivStrategy : public InstructionStrategyBase<OpCode::OP_DIV> {
    public:
        const char* name() const noexcept override { return "DIV"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MOD instruction
     * R[A] := R[B] % R[C]
     */
    class ModStrategy : public InstructionStrategyBase<OpCode::OP_MOD> {
    public:
        const char* name() const noexcept override { return "MOD"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_POW instruction
     * R[A] := R[B] ^ R[C]
     */
    class PowStrategy : public InstructionStrategyBase<OpCode::OP_POW> {
    public:
        const char* name() const noexcept override { return "POW"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_IDIV instruction
     * R[A] := R[B] // R[C] (integer division)
     */
    class IDivStrategy : public InstructionStrategyBase<OpCode::OP_IDIV> {
    public:
        const char* name() const noexcept override { return "IDIV"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_UNM instruction
     * R[A] := -R[B] (unary minus)
     */
    class UnmStrategy : public InstructionStrategyBase<OpCode::OP_UNM> {
    public:
        const char* name() const noexcept override { return "UNM"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_ADDI instruction
     * R[A] := R[B] + sC (add immediate)
     */
    class AddIStrategy : public InstructionStrategyBase<OpCode::OP_ADDI> {
    public:
        const char* name() const noexcept override { return "ADDI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_ADDK instruction
     * R[A] := R[B] + K[C] (add constant)
     */
    class AddKStrategy : public InstructionStrategyBase<OpCode::OP_ADDK> {
    public:
        const char* name() const noexcept override { return "ADDK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SUBK instruction
     * R[A] := R[B] - K[C] (subtract constant)
     */
    class SubKStrategy : public InstructionStrategyBase<OpCode::OP_SUBK> {
    public:
        const char* name() const noexcept override { return "SUBK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MULK instruction
     * R[A] := R[B] * K[C] (multiply constant)
     */
    class MulKStrategy : public InstructionStrategyBase<OpCode::OP_MULK> {
    public:
        const char* name() const noexcept override { return "MULK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MODK instruction
     * R[A] := R[B] % K[C] (modulo constant)
     */
    class ModKStrategy : public InstructionStrategyBase<OpCode::OP_MODK> {
    public:
        const char* name() const noexcept override { return "MODK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_POWK instruction
     * R[A] := R[B] ^ K[C] (power constant)
     */
    class PowKStrategy : public InstructionStrategyBase<OpCode::OP_POWK> {
    public:
        const char* name() const noexcept override { return "POWK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_DIVK instruction
     * R[A] := R[B] / K[C] (divide constant)
     */
    class DivKStrategy : public InstructionStrategyBase<OpCode::OP_DIVK> {
    public:
        const char* name() const noexcept override { return "DIVK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_IDIVK instruction
     * R[A] := R[B] // K[C] (integer divide constant)
     */
    class IDivKStrategy : public InstructionStrategyBase<OpCode::OP_IDIVK> {
    public:
        const char* name() const noexcept override { return "IDIVK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating arithmetic operation strategies
     */
    class ArithmeticStrategyFactory {
    public:
        /**
         * @brief Create all arithmetic operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        ArithmeticStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
