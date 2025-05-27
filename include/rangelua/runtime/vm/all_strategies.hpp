#pragma once

/**
 * @file all_strategies.hpp
 * @brief Comprehensive include for all VM instruction strategies
 * @version 0.1.0
 */

// Core strategy framework
#include "instruction_strategy.hpp"

// Instruction strategy categories
#include "load_strategies.hpp"
#include "arithmetic_strategies.hpp"
#include "comparison_strategies.hpp"
#include "table_strategies.hpp"
#include "control_flow_strategies.hpp"

// Additional strategy categories
#include "bitwise_strategies.hpp"
#include "upvalue_strategies.hpp"
#include "global_strategies.hpp"
#include "misc_strategies.hpp"

namespace rangelua::runtime {

    /**
     * @brief Master factory for creating all instruction strategies
     * 
     * Provides a single entry point for registering all available
     * instruction strategies with the VM strategy registry.
     */
    class AllStrategiesFactory {
    public:
        /**
         * @brief Register all instruction strategies
         * @param registry Registry to register strategies with
         */
        static void register_all_strategies(InstructionStrategyRegistry& registry);

        /**
         * @brief Get count of all available strategies
         * @return Total number of instruction strategies
         */
        static Size get_strategy_count() noexcept;

        /**
         * @brief Validate that all required opcodes have strategies
         * @param registry Registry to validate
         * @return True if all opcodes are covered
         */
        static bool validate_complete_coverage(const InstructionStrategyRegistry& registry) noexcept;

    private:
        AllStrategiesFactory() = default;
    };

}  // namespace rangelua::runtime
