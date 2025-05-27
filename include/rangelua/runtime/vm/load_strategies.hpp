#pragma once

/**
 * @file load_strategies.hpp
 * @brief Load operation instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_MOVE instruction
     * R[A] := R[B]
     */
    class MoveStrategy : public InstructionStrategyBase<OpCode::OP_MOVE> {
    public:
        const char* name() const noexcept override { return "MOVE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADI instruction
     * R[A] := sBx (signed integer)
     */
    class LoadIStrategy : public InstructionStrategyBase<OpCode::OP_LOADI> {
    public:
        const char* name() const noexcept override { return "LOADI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADF instruction
     * R[A] := sBx (float)
     */
    class LoadFStrategy : public InstructionStrategyBase<OpCode::OP_LOADF> {
    public:
        const char* name() const noexcept override { return "LOADF"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADK instruction
     * R[A] := K[Bx]
     */
    class LoadKStrategy : public InstructionStrategyBase<OpCode::OP_LOADK> {
    public:
        const char* name() const noexcept override { return "LOADK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADKX instruction
     * R[A] := K[extra arg]
     */
    class LoadKXStrategy : public InstructionStrategyBase<OpCode::OP_LOADKX> {
    public:
        const char* name() const noexcept override { return "LOADKX"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADFALSE instruction
     * R[A] := false
     */
    class LoadFalseStrategy : public InstructionStrategyBase<OpCode::OP_LOADFALSE> {
    public:
        const char* name() const noexcept override { return "LOADFALSE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LFALSESKIP instruction
     * R[A] := false; pc++
     */
    class LFalseSkipStrategy : public InstructionStrategyBase<OpCode::OP_LFALSESKIP> {
    public:
        const char* name() const noexcept override { return "LFALSESKIP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADTRUE instruction
     * R[A] := true
     */
    class LoadTrueStrategy : public InstructionStrategyBase<OpCode::OP_LOADTRUE> {
    public:
        const char* name() const noexcept override { return "LOADTRUE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LOADNIL instruction
     * R[A], R[A+1], ..., R[A+B] := nil
     */
    class LoadNilStrategy : public InstructionStrategyBase<OpCode::OP_LOADNIL> {
    public:
        const char* name() const noexcept override { return "LOADNIL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating load operation strategies
     */
    class LoadStrategyFactory {
    public:
        /**
         * @brief Create all load operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        LoadStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
