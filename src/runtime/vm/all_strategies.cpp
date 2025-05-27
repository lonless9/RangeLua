/**
 * @file all_strategies.cpp
 * @brief Implementation of master strategy factory
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/all_strategies.hpp>
#include <rangelua/utils/logger.hpp>

// Include all strategy headers
#include <rangelua/runtime/vm/load_strategies.hpp>
#include <rangelua/runtime/vm/arithmetic_strategies.hpp>
#include <rangelua/runtime/vm/comparison_strategies.hpp>
#include <rangelua/runtime/vm/table_strategies.hpp>
#include <rangelua/runtime/vm/control_flow_strategies.hpp>
#include <rangelua/runtime/vm/bitwise_strategies.hpp>
#include <rangelua/runtime/vm/upvalue_strategies.hpp>
#include <rangelua/runtime/vm/global_strategies.hpp>
#include <rangelua/runtime/vm/misc_strategies.hpp>

namespace rangelua::runtime {

    void AllStrategiesFactory::register_all_strategies(InstructionStrategyRegistry& registry) {
        VM_LOG_INFO("Registering all instruction strategies");

        try {
            // Register load operations
            LoadStrategyFactory::register_strategies(registry);

            // Register arithmetic operations
            ArithmeticStrategyFactory::register_strategies(registry);

            // Register comparison operations
            ComparisonStrategyFactory::register_strategies(registry);

            // Register table operations
            TableStrategyFactory::register_strategies(registry);

            // Register control flow operations
            ControlFlowStrategyFactory::register_strategies(registry);

            // Register bitwise operations
            BitwiseStrategyFactory::register_strategies(registry);

            // Register upvalue operations
            UpvalueStrategyFactory::register_strategies(registry);

            // Register global operations (currently empty for Lua 5.5)
            GlobalStrategyFactory::register_strategies(registry);

            // Register miscellaneous operations
            MiscStrategyFactory::register_strategies(registry);

            VM_LOG_INFO("Successfully registered all instruction strategies");

        } catch (const Exception& e) {
            VM_LOG_ERROR("Failed to register instruction strategies: {}", e.what());
            throw;
        } catch (const std::exception& e) {
            VM_LOG_ERROR("Failed to register instruction strategies: {}", e.what());
            throw;
        } catch (...) {
            VM_LOG_ERROR("Failed to register instruction strategies: unknown error");
            throw;
        }
    }

    Size AllStrategiesFactory::get_strategy_count() noexcept {
        // This is an estimate based on the number of opcodes in Lua 5.5
        // The actual count will be determined at runtime
        return static_cast<Size>(OpCode::NUM_OPCODES);
    }

    bool AllStrategiesFactory::validate_complete_coverage(const InstructionStrategyRegistry& registry) noexcept {
        VM_LOG_DEBUG("Validating instruction strategy coverage");

        Size missing_count = 0;
        Size total_opcodes = static_cast<Size>(OpCode::NUM_OPCODES);

        for (Size i = 0; i < total_opcodes; ++i) {
            OpCode opcode = static_cast<OpCode>(i);

            if (!registry.has_strategy(opcode)) {
                VM_LOG_WARN("Missing strategy for opcode {} ({})",
                             i, static_cast<int>(opcode));
                ++missing_count;
            }
        }

        if (missing_count > 0) {
            VM_LOG_WARN("Found {} missing strategies out of {} total opcodes",
                          missing_count, total_opcodes);
            return false;
        }

        VM_LOG_DEBUG("All {} opcodes have registered strategies", total_opcodes);
        return true;
    }

}  // namespace rangelua::runtime
