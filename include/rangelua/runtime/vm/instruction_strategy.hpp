#pragma once

/**
 * @file instruction_strategy.hpp
 * @brief Strategy pattern interface for VM instruction execution
 * @version 0.1.0
 */

#include <memory>
#include <unordered_map>

#include "../../backend/bytecode.hpp"
#include "../../core/error.hpp"
#include "../../core/types.hpp"

namespace rangelua::runtime {

    // Forward declarations
    class VirtualMachine;
    class RuntimeMemoryManager;
    class Value;

    /**
     * @brief VM execution context interface for instruction strategies
     *
     * Provides controlled access to VM state for instruction execution
     * without exposing the entire VM implementation.
     */
    class IVMContext {
    public:
        virtual ~IVMContext() = default;

        // Stack operations
        [[nodiscard]] virtual Value& stack_at(Register reg) = 0;
        [[nodiscard]] virtual const Value& stack_at(Register reg) const = 0;
        virtual void push(Value value) = 0;
        [[nodiscard]] virtual Value pop() = 0;
        [[nodiscard]] virtual const Value& top() const = 0;
        [[nodiscard]] virtual Size stack_size() const noexcept = 0;

        // Call frame operations
        [[nodiscard]] virtual Size instruction_pointer() const noexcept = 0;
        virtual void set_instruction_pointer(Size ip) noexcept = 0;
        virtual void adjust_instruction_pointer(std::int32_t offset) noexcept = 0;
        [[nodiscard]] virtual const backend::BytecodeFunction* current_function() const noexcept = 0;
        [[nodiscard]] virtual Size call_depth() const noexcept = 0;

        // Global variables
        [[nodiscard]] virtual Value get_global(const String& name) const = 0;
        virtual void set_global(const String& name, Value value) = 0;

        // Constants access
        [[nodiscard]] virtual Value get_constant(std::uint16_t index) const = 0;

        // Function calls
        virtual Status call_function(const Value& function, const std::vector<Value>& args,
                                   std::vector<Value>& results) = 0;
        virtual Result<std::vector<Value>> pcall(const Value& function,
                                                 const std::vector<Value>& args) = 0;
        virtual Result<std::vector<Value>> xpcall(const Value& function,
                                                  const Value& msgh,
                                                  const std::vector<Value>& args) = 0;
        virtual Status setup_call_frame(const backend::BytecodeFunction& function, Size arg_count) = 0;
        virtual Status return_from_function(Size result_count) = 0;

        // Error handling
        virtual void set_error(ErrorCode code) = 0;
        virtual void set_runtime_error(const String& message) = 0;
        virtual void trigger_runtime_error(const String& message) = 0;

        // Memory management
        virtual RuntimeMemoryManager& memory_manager() noexcept = 0;

        // Upvalue operations (for future implementation)
        [[nodiscard]] virtual Value get_upvalue(UpvalueIndex index) const = 0;
        virtual void set_upvalue(UpvalueIndex index, const Value& value) = 0;

        virtual VirtualMachine& get_vm() = 0;
    };

    /**
     * @brief Base strategy interface for VM instruction execution
     *
     * All instruction strategies must implement this interface.
     * Uses the Strategy design pattern to separate instruction logic
     * from the main VM execution loop.
     */
    class IInstructionStrategy {
    public:
        virtual ~IInstructionStrategy() = default;

        /**
         * @brief Execute the instruction
         * @param context VM execution context
         * @param instruction Raw instruction data
         * @return Execution status (success or error)
         */
        virtual Status execute(IVMContext& context, Instruction instruction) = 0;

        /**
         * @brief Get the opcode this strategy handles
         * @return OpCode value
         */
        [[nodiscard]] virtual OpCode opcode() const noexcept = 0;

        /**
         * @brief Get strategy name for debugging
         * @return Strategy name
         */
        virtual const char* name() const noexcept = 0;
    };

    /**
     * @brief Template base class for instruction strategies
     *
     * Provides common functionality and type safety for specific instruction types.
     * Derived classes only need to implement execute_impl().
     */
    template<OpCode Op>
    class InstructionStrategyBase : public IInstructionStrategy {
    public:
        OpCode opcode() const noexcept final { return Op; }

        Status execute(IVMContext& context, Instruction instruction) final {
            try {
                return execute_impl(context, instruction);
            } catch (const RuntimeError& e) {
                // Re-throw runtime errors to be caught by pcall or the main execution loop.
                throw;
            } catch (const Exception& e) {
                context.set_error(e.code());
                return e.code();
            } catch (const std::exception& e) {
                context.set_runtime_error(String("Instruction execution error: ") + e.what());
                return ErrorCode::RUNTIME_ERROR;
            } catch (...) {
                context.set_runtime_error("Unknown error in instruction execution");
                return ErrorCode::UNKNOWN_ERROR;
            }
        }

    protected:
        /**
         * @brief Implement instruction-specific execution logic
         * @param context VM execution context
         * @param instruction Raw instruction data
         * @return Execution status
         */
        virtual Status execute_impl(IVMContext& context, Instruction instruction) = 0;
    };

    /**
     * @brief Strategy registry for managing instruction strategies
     *
     * Provides fast O(1) lookup of instruction strategies by opcode.
     * Uses modern C++20 features for type safety and performance.
     */
    class InstructionStrategyRegistry {
    public:
        InstructionStrategyRegistry();
        ~InstructionStrategyRegistry() = default;

        // Non-copyable, movable
        InstructionStrategyRegistry(const InstructionStrategyRegistry&) = delete;
        InstructionStrategyRegistry& operator=(const InstructionStrategyRegistry&) = delete;
        InstructionStrategyRegistry(InstructionStrategyRegistry&&) noexcept = default;
        InstructionStrategyRegistry& operator=(InstructionStrategyRegistry&&) noexcept = default;

        /**
         * @brief Register a strategy for an opcode
         * @param strategy Unique pointer to strategy implementation
         */
        void register_strategy(std::unique_ptr<IInstructionStrategy> strategy);

        /**
         * @brief Get strategy for opcode
         * @param opcode Instruction opcode
         * @return Strategy pointer or nullptr if not found
         */
        IInstructionStrategy* get_strategy(OpCode opcode) const noexcept;

        /**
         * @brief Execute instruction using registered strategy
         * @param context VM execution context
         * @param opcode Instruction opcode
         * @param instruction Raw instruction data
         * @return Execution status
         */
        Status execute_instruction(IVMContext& context, OpCode opcode, Instruction instruction) const;

        /**
         * @brief Check if strategy is registered for opcode
         * @param opcode Instruction opcode
         * @return True if strategy exists
         */
        bool has_strategy(OpCode opcode) const noexcept;

        /**
         * @brief Get count of registered strategies
         * @return Number of registered strategies
         */
        Size strategy_count() const noexcept;

    private:
        std::unordered_map<OpCode, std::unique_ptr<IInstructionStrategy>> strategies_;

        /**
         * @brief Initialize all instruction strategies
         */
        void initialize_strategies();
    };

    /**
     * @brief Factory for creating instruction strategy registry
     */
    class InstructionStrategyFactory {
    public:
        /**
         * @brief Create a fully initialized strategy registry
         * @return Unique pointer to registry
         */
        static std::unique_ptr<InstructionStrategyRegistry> create_registry();

    private:
        InstructionStrategyFactory() = default;
    };

}  // namespace rangelua::runtime
