#pragma once

/**
 * @file misc_strategies.hpp
 * @brief Miscellaneous instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_NOT instruction
     * R[A] := not R[B]
     */
    class NotStrategy : public InstructionStrategyBase<OpCode::OP_NOT> {
    public:
        const char* name() const noexcept override { return "NOT"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LEN instruction
     * R[A] := #R[B] (length operator)
     */
    class LenStrategy : public InstructionStrategyBase<OpCode::OP_LEN> {
    public:
        const char* name() const noexcept override { return "LEN"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_CONCAT instruction
     * R[A] := R[A].. ... ..R[A + B - 1]
     */
    class ConcatStrategy : public InstructionStrategyBase<OpCode::OP_CONCAT> {
    public:
        const char* name() const noexcept override { return "CONCAT"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_VARARG instruction
     * R[A], R[A+1], ..., R[A+C-2] = vararg
     */
    class VarargStrategy : public InstructionStrategyBase<OpCode::OP_VARARG> {
    public:
        const char* name() const noexcept override { return "VARARG"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_VARARGPREP instruction
     * (adjust vararg parameters)
     */
    class VarargPrepStrategy : public InstructionStrategyBase<OpCode::OP_VARARGPREP> {
    public:
        const char* name() const noexcept override { return "VARARGPREP"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MMBIN instruction
     * call C metamethod over R[A] and R[B]
     */
    class MmbinStrategy : public InstructionStrategyBase<OpCode::OP_MMBIN> {
    public:
        const char* name() const noexcept override { return "MMBIN"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MMBINI instruction
     * call C metamethod over R[A] and sB
     */
    class MmbiniStrategy : public InstructionStrategyBase<OpCode::OP_MMBINI> {
    public:
        const char* name() const noexcept override { return "MMBINI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_MMBINK instruction
     * call C metamethod over R[A] and K[B]
     */
    class MmbinkStrategy : public InstructionStrategyBase<OpCode::OP_MMBINK> {
    public:
        const char* name() const noexcept override { return "MMBINK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_EXTRAARG instruction
     * extra (larger) argument for previous opcode
     */
    class ExtraArgStrategy : public InstructionStrategyBase<OpCode::OP_EXTRAARG> {
    public:
        const char* name() const noexcept override { return "EXTRAARG"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating miscellaneous operation strategies
     */
    class MiscStrategyFactory {
    public:
        /**
         * @brief Create all miscellaneous operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        MiscStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
