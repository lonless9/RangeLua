/**
 * @file instruction_strategy.cpp
 * @brief Implementation of instruction strategy registry and factory
 * @version 0.1.0
 */

#include <rangelua/runtime/vm/instruction_strategy.hpp>
#include <rangelua/runtime/vm/all_strategies.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // InstructionStrategyRegistry implementation
    InstructionStrategyRegistry::InstructionStrategyRegistry() {
        initialize_strategies();
    }

    void InstructionStrategyRegistry::register_strategy(std::unique_ptr<IInstructionStrategy> strategy) {
        if (!strategy) {
            VM_LOG_ERROR("Attempted to register null strategy");
            return;
        }

        OpCode opcode = strategy->opcode();
        const char* name = strategy->name();

        if (strategies_.contains(opcode)) {
            VM_LOG_WARN("Overriding existing strategy for opcode {} ({})",
                          static_cast<int>(opcode), name);
        }

        VM_LOG_DEBUG("Registering strategy for opcode {} ({})",
                     static_cast<int>(opcode), name);

        strategies_[opcode] = std::move(strategy);
    }

    IInstructionStrategy* InstructionStrategyRegistry::get_strategy(OpCode opcode) const noexcept {
        auto it = strategies_.find(opcode);
        return it != strategies_.end() ? it->second.get() : nullptr;
    }

    Status InstructionStrategyRegistry::execute_instruction(IVMContext& context,
                                                           OpCode opcode,
                                                           Instruction instruction) const {
        auto* strategy = get_strategy(opcode);
        if (!strategy) {
            VM_LOG_ERROR("No strategy found for opcode {}", static_cast<int>(opcode));
            context.set_runtime_error("Unimplemented instruction");
            return ErrorCode::RUNTIME_ERROR;
        }

        VM_LOG_DEBUG("Executing instruction {} using strategy {}",
                     static_cast<int>(opcode), strategy->name());

        try {
            return strategy->execute(context, instruction);
        } catch (const Exception& e) {
            VM_LOG_ERROR("Strategy execution failed: {}", e.what());
            context.set_error(e.code());
            return e.code();
        } catch (const std::exception& e) {
            VM_LOG_ERROR("Strategy execution failed with std::exception: {}", e.what());
            context.set_runtime_error(String("Strategy execution error: ") + e.what());
            return ErrorCode::RUNTIME_ERROR;
        } catch (...) {
            VM_LOG_ERROR("Strategy execution failed with unknown exception");
            context.set_runtime_error("Unknown error in strategy execution");
            return ErrorCode::UNKNOWN_ERROR;
        }
    }

    bool InstructionStrategyRegistry::has_strategy(OpCode opcode) const noexcept {
        return strategies_.contains(opcode);
    }

    Size InstructionStrategyRegistry::strategy_count() const noexcept {
        return strategies_.size();
    }

    void InstructionStrategyRegistry::initialize_strategies() {
        VM_LOG_INFO("Initializing instruction strategy registry");

        try {
            // Register all instruction strategies using the master factory
            AllStrategiesFactory::register_all_strategies(*this);

            VM_LOG_INFO("Successfully registered {} instruction strategies", strategy_count());

            // Validate that we have complete coverage
            if (AllStrategiesFactory::validate_complete_coverage(*this)) {
                VM_LOG_INFO("All instruction opcodes have registered strategies");
            } else {
                VM_LOG_WARN("Some instruction opcodes are missing strategies");
            }

        } catch (const Exception& e) {
            VM_LOG_ERROR("Failed to initialize instruction strategies: {}", e.what());
            throw;
        } catch (const std::exception& e) {
            VM_LOG_ERROR("Failed to initialize instruction strategies: {}", e.what());
            throw;
        } catch (...) {
            VM_LOG_ERROR("Failed to initialize instruction strategies: unknown error");
            throw;
        }
    }

    // InstructionStrategyFactory implementation
    std::unique_ptr<InstructionStrategyRegistry> InstructionStrategyFactory::create_registry() {
        VM_LOG_DEBUG("Creating instruction strategy registry");

        try {
            auto registry = std::make_unique<InstructionStrategyRegistry>();

            VM_LOG_DEBUG("Created strategy registry with {} strategies",
                        registry->strategy_count());

            return registry;

        } catch (const Exception& e) {
            VM_LOG_ERROR("Failed to create strategy registry: {}", e.what());
            throw;
        } catch (const std::exception& e) {
            VM_LOG_ERROR("Failed to create strategy registry: {}", e.what());
            throw;
        } catch (...) {
            VM_LOG_ERROR("Failed to create strategy registry: unknown error");
            throw;
        }
    }

}  // namespace rangelua::runtime
