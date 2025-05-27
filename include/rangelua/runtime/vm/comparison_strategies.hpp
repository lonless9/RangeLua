#pragma once

/**
 * @file comparison_strategies.hpp
 * @brief Comparison operation instruction strategies
 * @version 0.1.0
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Strategy for OP_EQ instruction
     * if ((R[A] == R[B]) ~= k) then pc++
     */
    class EqStrategy : public InstructionStrategyBase<OpCode::OP_EQ> {
    public:
        const char* name() const noexcept override { return "EQ"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LT instruction
     * if ((R[A] < R[B]) ~= k) then pc++
     */
    class LtStrategy : public InstructionStrategyBase<OpCode::OP_LT> {
    public:
        const char* name() const noexcept override { return "LT"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LE instruction
     * if ((R[A] <= R[B]) ~= k) then pc++
     */
    class LeStrategy : public InstructionStrategyBase<OpCode::OP_LE> {
    public:
        const char* name() const noexcept override { return "LE"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_EQK instruction
     * if ((R[A] == K[B]) ~= k) then pc++
     */
    class EqKStrategy : public InstructionStrategyBase<OpCode::OP_EQK> {
    public:
        const char* name() const noexcept override { return "EQK"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_EQI instruction
     * if ((R[A] == sB) ~= k) then pc++
     */
    class EqIStrategy : public InstructionStrategyBase<OpCode::OP_EQI> {
    public:
        const char* name() const noexcept override { return "EQI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LTI instruction
     * if ((R[A] < sB) ~= k) then pc++
     */
    class LtIStrategy : public InstructionStrategyBase<OpCode::OP_LTI> {
    public:
        const char* name() const noexcept override { return "LTI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_LEI instruction
     * if ((R[A] <= sB) ~= k) then pc++
     */
    class LeIStrategy : public InstructionStrategyBase<OpCode::OP_LEI> {
    public:
        const char* name() const noexcept override { return "LEI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_GTI instruction
     * if ((R[A] > sB) ~= k) then pc++
     */
    class GtIStrategy : public InstructionStrategyBase<OpCode::OP_GTI> {
    public:
        const char* name() const noexcept override { return "GTI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_GEI instruction
     * if ((R[A] >= sB) ~= k) then pc++
     */
    class GeIStrategy : public InstructionStrategyBase<OpCode::OP_GEI> {
    public:
        const char* name() const noexcept override { return "GEI"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TEST instruction
     * if not (R[A] <=> C) then pc++
     */
    class TestStrategy : public InstructionStrategyBase<OpCode::OP_TEST> {
    public:
        const char* name() const noexcept override { return "TEST"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Strategy for OP_TESTSET instruction
     * if (R[B] <=> C) then R[A] := R[B] else pc++
     */
    class TestSetStrategy : public InstructionStrategyBase<OpCode::OP_TESTSET> {
    public:
        const char* name() const noexcept override { return "TESTSET"; }

    protected:
        Status execute_impl(IVMContext& context, Instruction instruction) override;
    };

    /**
     * @brief Factory for creating comparison operation strategies
     */
    class ComparisonStrategyFactory {
    public:
        /**
         * @brief Create all comparison operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        ComparisonStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
