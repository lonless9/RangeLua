#pragma once

/**
 * @file vm.hpp
 * @brief Virtual machine execution engine
 * @version 0.1.0
 */

#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../backend/bytecode.hpp"
#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/types.hpp"
#include "environment.hpp"
#include "memory.hpp"
#include "value.hpp"
#include "vm/instruction_strategy.hpp"

namespace rangelua::runtime {

    // Forward declarations for strategy pattern
    class IVMContext;
    class InstructionStrategyRegistry;

    /**
     * @brief Call frame for function calls
     */
    struct CallFrame {
        const backend::BytecodeFunction* function = nullptr;
        GCPtr<Function> closure;  // Closure for upvalue access (default constructed to empty)
        Size instruction_pointer = 0;
        Size stack_base = 0;
        Size local_count = 0;
        bool is_tail_call = false;

        CallFrame() = default;
        CallFrame(const backend::BytecodeFunction* func, Size stack_base, Size locals)
            : function(func), stack_base(stack_base), local_count(locals) {}
        CallFrame(const backend::BytecodeFunction* func,
                  const GCPtr<Function>& closure,
                  Size stack_base,
                  Size locals)
            : function(func), closure(closure), stack_base(stack_base), local_count(locals) {}
    };

    /**
     * @brief VM execution state
     */
    enum class VMState : std::uint8_t { Ready, Running, Suspended, Error, Finished };

    /**
     * @brief VM configuration
     */
    struct VMConfig {
        Size stack_size = 1024;
        Size call_stack_size = 256;
        Size max_recursion_depth = 1000;
        bool enable_debugging = false;
        bool enable_profiling = false;
        bool enable_tail_call_optimization = true;
        bool enable_computed_goto = true;
    };

    /**
     * @brief Virtual machine for executing Lua bytecode
     */
    class VirtualMachine : public IVMContext {
        friend class ExecutionContext;
        friend class VMDebugger;

    public:
        explicit VirtualMachine(VMConfig config = {});
        explicit VirtualMachine(RuntimeMemoryManager& memory_manager, VMConfig config = {});

        ~VirtualMachine() = default;

        // Non-copyable, movable
        VirtualMachine(const VirtualMachine&) = delete;
        VirtualMachine& operator=(const VirtualMachine&) = delete;
        VirtualMachine(VirtualMachine&&) noexcept = default;
        VirtualMachine& operator=(VirtualMachine&&) noexcept = default;

        /**
         * @brief Execute bytecode function
         * @param function Function to execute
         * @param args Function arguments
         * @return Execution result
         */
        Result<std::vector<Value>> execute(const backend::BytecodeFunction& function,
                                           const std::vector<Value>& args = {});

        /**
         * @brief Execute single instruction step
         * @return Success or error
         */
        Status step();

        /**
         * @brief Call function with arguments
         * @param function Function to call
         * @param args Arguments
         * @return Call result
         */
        Result<std::vector<Value>> call(const Value& function, const std::vector<Value>& args);

        /**
         * @brief Resume suspended execution
         * @return Execution result
         */
        Result<std::vector<Value>> resume();

        /**
         * @brief Suspend execution
         */
        void suspend();

        /**
         * @brief Reset VM state
         */
        void reset();

        /**
         * @brief Get current VM state
         */
        VMState state() const noexcept { return state_; }

        /**
         * @brief Check if VM is running
         */
        bool is_running() const noexcept { return state_ == VMState::Running; }

        /**
         * @brief Check if VM is suspended
         */
        bool is_suspended() const noexcept { return state_ == VMState::Suspended; }

        /**
         * @brief Check if VM has finished
         */
        bool is_finished() const noexcept { return state_ == VMState::Finished; }

        /**
         * @brief Check if VM has error
         */
        bool has_error() const noexcept { return state_ == VMState::Error; }

        /**
         * @brief Get last error code
         */
        ErrorCode last_error() const noexcept { return last_error_; }

        /**
         * @brief Get stack size
         */
        Size stack_size() const noexcept override { return stack_top_; }

        /**
         * @brief Get call stack depth
         */
        Size call_depth() const noexcept override { return call_stack_.size(); }

        /**
         * @brief Get current instruction pointer
         */
        Size instruction_pointer() const noexcept override;

        /**
         * @brief Get current function
         */
        const backend::BytecodeFunction* current_function() const noexcept override;

        // IVMContext interface implementation
        /**
         * @brief Push value onto stack
         */
        void push(Value value) override;

        /**
         * @brief Pop value from stack
         */
        Value pop() override;

        /**
         * @brief Peek at top of stack
         */
        const Value& top() const override;

        /**
         * @brief Get stack value at register
         */
        Value& stack_at(Register reg) override;
        const Value& stack_at(Register reg) const override;

        /**
         * @brief Set instruction pointer
         */
        void set_instruction_pointer(Size ip) noexcept override;

        /**
         * @brief Adjust instruction pointer by offset
         */
        void adjust_instruction_pointer(std::int32_t offset) noexcept override;

        /**
         * @brief Get global value
         */
        Value get_global(const String& name) const override;

        /**
         * @brief Set global value
         */
        void set_global(const String& name, Value value) override;

        /**
         * @brief Get constant value
         */
        Value get_constant(std::uint16_t index) const override;

        /**
         * @brief Call function with arguments
         */
        Status call_function(const Value& function,
                             const std::vector<Value>& args,
                             std::vector<Value>& results) override;

        /**
         * @brief Setup call frame
         */
        Status setup_call_frame(const backend::BytecodeFunction& function, Size arg_count) override;

        /**
         * @brief Return from function
         */
        Status return_from_function(Size result_count) override;

        /**
         * @brief Set error code
         */
        void set_error(ErrorCode code) override;

        /**
         * @brief Set runtime error
         */
        void set_runtime_error(const String& message) override;

        /**
         * @brief Get memory manager
         */
        RuntimeMemoryManager& memory_manager() noexcept override { return *memory_manager_; }

        /**
         * @brief Get upvalue
         */
        Value get_upvalue(UpvalueIndex index) const override;

        /**
         * @brief Set upvalue
         */
        void set_upvalue(UpvalueIndex index, const Value& value) override;

        // Additional VM-specific methods
        /**
         * @brief Get stack value at index
         */
        const Value& get_stack(Size index) const;

        /**
         * @brief Set stack value at index
         */
        void set_stack(Size index, Value value);

        /**
         * @brief Get VM configuration
         */
        const VMConfig& config() const noexcept { return config_; }

        /**
         * @brief Get global table from environment
         */
        GCPtr<Table> get_global_table() const;

        /**
         * @brief Get environment registry
         */
        Registry* get_registry() const noexcept;

        /**
         * @brief Trigger runtime error for testing
         */
        void trigger_runtime_error(const String& message);

    private:
        VMConfig config_;
        VMState state_ = VMState::Ready;

        // Memory management
        std::unique_ptr<RuntimeMemoryManager> owned_memory_manager_;
        RuntimeMemoryManager* memory_manager_;

        // Strategy pattern for instruction execution
        std::unique_ptr<InstructionStrategyRegistry> strategy_registry_;

        // Execution state
        std::vector<Value> stack_;
        std::vector<CallFrame> call_stack_;

        // Environment and global table management
        std::unique_ptr<Registry> registry_;
        std::unique_ptr<Environment> environment_;

        // Upvalue management
        class Upvalue* open_upvalues_ = nullptr;  // Linked list of open upvalues

        // Current execution context
        Size stack_top_ = 0;
        ErrorCode last_error_ = ErrorCode::SUCCESS;

        // Instruction execution methods
        Status execute_instruction(OpCode opcode, Instruction instruction);

        // Stack operations
        void ensure_stack_size(Size size);

        // Function call implementations
        Result<std::vector<Value>> call_lua_function(GCPtr<Function> function,
                                                     const std::vector<Value>& args);

        // Legacy call operations (for backward compatibility)
        Status call_function(const Value& function, Size arg_count, Size result_count);

        // Legacy instruction implementations (deprecated - use strategy pattern instead)

        // Upvalue management
        class Upvalue* find_upvalue(Value* stack_location);
        void close_upvalues(Value* level);

        // Helper methods
        Value constant_to_value(const backend::ConstantValue& constant);
    };

    /**
     * @brief VM execution context for coroutines
     */
    class ExecutionContext {
    public:
        explicit ExecutionContext(VirtualMachine& vm);

        /**
         * @brief Save current execution state
         */
        void save_state();

        /**
         * @brief Restore execution state
         */
        void restore_state();

        /**
         * @brief Check if context is valid
         */
        [[nodiscard]] bool is_valid() const noexcept;

    private:
        VirtualMachine& vm_;
        VMState saved_state_;
        std::vector<Value> saved_stack_;
        std::vector<CallFrame> saved_call_stack_;
        Size saved_stack_top_;
        bool is_saved_ = false;
    };

    /**
     * @brief VM debugging interface
     */
    class VMDebugger {
    public:
        explicit VMDebugger(VirtualMachine& vm);

        /**
         * @brief Set breakpoint at instruction
         */
        void set_breakpoint(Size instruction);

        /**
         * @brief Remove breakpoint
         */
        void remove_breakpoint(Size instruction);

        /**
         * @brief Step single instruction
         */
        Status step_instruction();

        /**
         * @brief Step over function calls
         */
        Status step_over();

        /**
         * @brief Step into function calls
         */
        Status step_into();

        /**
         * @brief Continue execution
         */
        Status continue_execution();

        /**
         * @brief Get current stack trace
         */
        std::vector<String> get_stack_trace() const;

        /**
         * @brief Get local variables
         */
        std::unordered_map<String, Value> get_locals() const;

    private:
        VirtualMachine& vm_;
        std::unordered_set<Size> breakpoints_;
        bool is_debugging_ = false;
    };

}  // namespace rangelua::runtime

// Concept verification (commented out until concepts are properly implemented)
// static_assert(rangelua::concepts::VirtualMachine<rangelua::runtime::VirtualMachine>);
