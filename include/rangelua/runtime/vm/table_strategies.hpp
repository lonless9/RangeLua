#pragma once

/**
 * @file table_strategies.hpp
 * @brief Table operation instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_NEWTABLE instruction
     * R[A] := {} (new table)
     */
    class NewTableStrategy : public InstructionStrategyBase<OpCode::OP_NEWTABLE> {
    public:
        const char* name() const noexcept override { return "NEWTABLE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_GETTABLE instruction
     * R[A] := R[B][R[C]]
     */
    class GetTableStrategy : public InstructionStrategyBase<OpCode::OP_GETTABLE> {
    public:
        const char* name() const noexcept override { return "GETTABLE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SETTABLE instruction
     * R[A][R[B]] := R[C]
     */
    class SetTableStrategy : public InstructionStrategyBase<OpCode::OP_SETTABLE> {
    public:
        const char* name() const noexcept override { return "SETTABLE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_GETTABUP instruction
     * R[A] := UpValue[B][R[C]]
     */
    class GetTabUpStrategy : public InstructionStrategyBase<OpCode::OP_GETTABUP> {
    public:
        const char* name() const noexcept override { return "GETTABUP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SETTABUP instruction
     * UpValue[A][R[B]] := R[C]
     */
    class SetTabUpStrategy : public InstructionStrategyBase<OpCode::OP_SETTABUP> {
    public:
        const char* name() const noexcept override { return "SETTABUP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_GETI instruction
     * R[A] := R[B][C]
     */
    class GetIStrategy : public InstructionStrategyBase<OpCode::OP_GETI> {
    public:
        const char* name() const noexcept override { return "GETI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SETI instruction
     * R[A][B] := R[C]
     */
    class SetIStrategy : public InstructionStrategyBase<OpCode::OP_SETI> {
    public:
        const char* name() const noexcept override { return "SETI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_GETFIELD instruction
     * R[A] := R[B][K[C]]
     */
    class GetFieldStrategy : public InstructionStrategyBase<OpCode::OP_GETFIELD> {
    public:
        const char* name() const noexcept override { return "GETFIELD"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SETFIELD instruction
     * R[A][K[B]] := R[C]
     */
    class SetFieldStrategy : public InstructionStrategyBase<OpCode::OP_SETFIELD> {
    public:
        const char* name() const noexcept override { return "SETFIELD"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SELF instruction
     * R[A+1] := R[B]; R[A] := R[B][R[C]]
     */
    class SelfStrategy : public InstructionStrategyBase<OpCode::OP_SELF> {
    public:
        const char* name() const noexcept override { return "SELF"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_SETLIST instruction
     * R[A][vC+i] := R[A+i], 1 <= i <= vB
     */
    class SetListStrategy : public InstructionStrategyBase<OpCode::OP_SETLIST> {
    public:
        const char* name() const noexcept override { return "SETLIST"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating table operation strategies
     */
    class TableStrategyFactory {
    public:
        /**
         * @brief Create all table operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        TableStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
