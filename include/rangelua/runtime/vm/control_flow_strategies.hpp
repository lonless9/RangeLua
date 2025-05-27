#pragma once

/**
 * @file control_flow_strategies.hpp
 * @brief Control flow instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_JMP instruction
     * pc += sJ
     */
    class JmpStrategy : public InstructionStrategyBase<OpCode::OP_JMP> {
    public:
        const char* name() const noexcept override { return "JMP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_CALL instruction
     * R[A], ... ,R[A+C-2] := R[A](R[A+1], ... ,R[A+B-1])
     */
    class CallStrategy : public InstructionStrategyBase<OpCode::OP_CALL> {
    public:
        const char* name() const noexcept override { return "CALL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TAILCALL instruction
     * return R[A](R[A+1], ... ,R[A+B-1])
     */
    class TailCallStrategy : public InstructionStrategyBase<OpCode::OP_TAILCALL> {
    public:
        const char* name() const noexcept override { return "TAILCALL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_RETURN instruction
     * return R[A], ... ,R[A+B-2]
     */
    class ReturnStrategy : public InstructionStrategyBase<OpCode::OP_RETURN> {
    public:
        const char* name() const noexcept override { return "RETURN"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_RETURN0 instruction
     * return
     */
    class Return0Strategy : public InstructionStrategyBase<OpCode::OP_RETURN0> {
    public:
        const char* name() const noexcept override { return "RETURN0"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_RETURN1 instruction
     * return R[A]
     */
    class Return1Strategy : public InstructionStrategyBase<OpCode::OP_RETURN1> {
    public:
        const char* name() const noexcept override { return "RETURN1"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_FORLOOP instruction
     * update counters; if loop continues then pc-=Bx;
     */
    class ForLoopStrategy : public InstructionStrategyBase<OpCode::OP_FORLOOP> {
    public:
        const char* name() const noexcept override { return "FORLOOP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_FORPREP instruction
     * <check values and prepare counters>; if not to run then pc+=Bx+1;
     */
    class ForPrepStrategy : public InstructionStrategyBase<OpCode::OP_FORPREP> {
    public:
        const char* name() const noexcept override { return "FORPREP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TFORPREP instruction
     * create upvalue for R[A + 3]; pc+=Bx
     */
    class TForPrepStrategy : public InstructionStrategyBase<OpCode::OP_TFORPREP> {
    public:
        const char* name() const noexcept override { return "TFORPREP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TFORCALL instruction
     * R[A+4], ... ,R[A+3+C] := R[A](R[A+1], R[A+2]);
     */
    class TForCallStrategy : public InstructionStrategyBase<OpCode::OP_TFORCALL> {
    public:
        const char* name() const noexcept override { return "TFORCALL"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TFORLOOP instruction
     * if R[A+2] ~= nil then { R[A]=R[A+2]; pc -= Bx }
     */
    class TForLoopStrategy : public InstructionStrategyBase<OpCode::OP_TFORLOOP> {
    public:
        const char* name() const noexcept override { return "TFORLOOP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_CLOSE instruction
     * close all upvalues >= R[A]
     */
    class CloseStrategy : public InstructionStrategyBase<OpCode::OP_CLOSE> {
    public:
        const char* name() const noexcept override { return "CLOSE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TBC instruction
     * mark variable A "to be closed"
     */
    class TbcStrategy : public InstructionStrategyBase<OpCode::OP_TBC> {
    public:
        const char* name() const noexcept override { return "TBC"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating control flow operation strategies
     */
    class ControlFlowStrategyFactory {
    public:
        /**
         * @brief Create all control flow operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        ControlFlowStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
