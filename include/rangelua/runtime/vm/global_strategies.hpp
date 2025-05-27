#pragma once

/**
 * @file global_strategies.hpp
 * @brief Global variable operation instruction strategies (placeholder)
 * @version 0.1.0
 *
 * Note: Lua 5.5 doesn't have dedicated global opcodes.
 * Global access is handled through GETTABUP/SETTABUP with _ENV upvalue.
 */

#include "instruction_strategy.hpp"
#include "../../backend/bytecode.hpp"

namespace rangelua::runtime {

    /**
     * @brief Factory for creating global variable operation strategies
     *
     * Currently empty as Lua 5.5 handles globals through upvalue operations.
     */
    class GlobalStrategyFactory {
    public:
        /**
         * @brief Create all global variable operation strategies
         * @param registry Registry to register strategies with
         */
        static void register_strategies(InstructionStrategyRegistry& registry);

    private:
        GlobalStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
