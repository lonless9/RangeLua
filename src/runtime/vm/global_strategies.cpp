/**
 * @file global_strategies.cpp
 * @brief Implementation of global variable operation instruction strategies
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/global_strategies.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // GlobalStrategyFactory implementation
    void GlobalStrategyFactory::register_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_DEBUG("Registering global operation strategies");
        
        // Lua 5.5 doesn't have dedicated global opcodes
        // Global access is handled through GETTABUP/SETTABUP with _ENV upvalue
        // So this factory is currently empty
        
        VM_LOG_DEBUG("Registered {} global operation strategies (Lua 5.5 uses upvalue operations)", 0);
    }

}  // namespace rangelua::runtime
