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
    class InstructionStrategyRegistry;

    /**
     * @brief Call frame for function calls
     */
    struct CallFrame {
        const backend::BytecodeFunction* function = nullptr;
        std::unique_ptr<const backend::BytecodeFunction> owned_function; // Owns the function if created on the fly
        GCPtr<Function> closure{};                                       // Closure for upvalue access (default constructed to empty)
        Size instruction_pointer = 0;
        Size stack_base = 0;
        Size local_count = 0;
        bool is_tail_call = false;
        bool is_protected_call = false;  // Is this a protected call boundary?
        int msgh = 0;                    // Stack index of the message handler for xpcall

        // Vararg support
        Size parameter_count = 0;  // Number of declared parameters
        Size argument_count = 0;   // Number of actual arguments passed
        Size vararg_base = 0;      // Stack position where varargs start
        bool has_varargs = false;  // Whether function accepts varargs

        CallFrame() = default;
        // Ensure move semantics are correctly handled for unique_ptr
        CallFrame(CallFrame&&) noexcept = default;
        CallFrame& operator=(CallFrame&&) noexcept = default;
        CallFrame(const CallFrame&) = delete;
        CallFrame& operator=(const CallFrame&) = delete;


        /**
         * @brief Get number of extra arguments (varargs)
         */
        [[nodiscard]] Size vararg_count() const noexcept {
            return (argument_count > parameter_count) ? (argument_count - parameter_count) : 0;
        }

        /**
         * @brief Check if there are varargs available
         */
        [[nodiscard]] bool has_vararg_values() const noexcept {
            return has_varargs && vararg_count() > 0;
        }
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

        Result<std::vector<Value>> pcall(const Value& function,
                                           const std::vector<Value>& args) override;

        Result<std::vector<Value>> xpcall(const Value& function,
                                            const Value& msgh,
                                            const std::vector<Value>& args) override;

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
        [[nodiscard]] VMState state() const noexcept { return state_; }

        /**
         * @brief Check if VM is running
         */
        [[nodiscard]] bool is_running() const noexcept { return state_ == VMState::Running; }

        /**
         * @brief Check if VM is suspended
         */
        [[nodiscard]] bool is_suspended() const noexcept { return state_ == VMState::Suspended; }

        /**
         * @brief Check if VM has finished
         */
        [[nodiscard]] bool is_finished() const noexcept { return state_ == VMState::Finished; }

        /**
         * @brief Check if VM has error
         */
        [[nodiscard]] bool has_error() const noexcept { return state_ == VMState::Error; }

        /**
         * @brief Get last error code
         */
        [[nodiscard]] ErrorCode last_error() const noexcept { return last_error_; }

        /**
         * @brief Get stack size
         */
        [[nodiscard]] Size stack_size() const noexcept override { return stack_top_; }

        /**
         * @brief Get call stack depth
         */
        [[nodiscard]] Size call_depth() const noexcept override { return call_stack_.size(); }

        /**
         * @brief Get current instruction pointer
         */
        [[nodiscard]] Size instruction_pointer() const noexcept override;

        /**
         * @brief Get current function
         */
        [[nodiscard]] const backend::BytecodeFunction* current_function() const noexcept override;

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
        [[nodiscard]] const Value& top() const override;

        /**
         * @brief Get stack value at register
         */
        Value& stack_at(Register reg) override;
        [[nodiscard]] const Value& stack_at(Register reg) const override;

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
        [[nodiscard]] Value get_global(const String& name) const override;

        /**
         * @brief Set global value
         */
        void set_global(const String& name, Value value) override;

        /**
         * @brief Get constant value
         */
        [[nodiscard]] Value get_constant(std::uint16_t index) const override;

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
         * @brief Setup call frame for a bytecode function, taking ownership
         */
        Status setup_call_frame(std::unique_ptr<const backend::BytecodeFunction> function, GCPtr<Function> closure, Size arg_count, Size stack_base);

        /**
         * @brief Return from function
         */
        Status return_from_function(Size result_count) override;

        /**
         * @brief Return from function with specific return value start register
         */
        Status return_from_function(Register return_start, Size result_count);

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
        [[nodiscard]] Value get_upvalue(UpvalueIndex index) const override;

        /**
         * @brief Set upvalue
         */
        void set_upvalue(UpvalueIndex index, const Value& value) override;

        VirtualMachine& get_vm() override { return *this; }

         // Additional VM-specific methods
        /**
         * @brief Get stack value at index
         */
        [[nodiscard]] const Value& get_stack(Size index) const;

        /**
         * @brief Set stack value at index
         */
        void set_stack(Size index, const Value& value);

        /**
         * @brief Get VM configuration
         */
        [[nodiscard]] const VMConfig& config() const noexcept { return config_; }

        /**
         * @brief Get global table from environment
         */
        [[nodiscard]] GCPtr<Table> get_global_table() const;

        /**
         * @brief Get environment registry
         */
        [[nodiscard]] Registry* get_registry() const noexcept;

        /**
         * @brief Trigger runtime error for testing
         */
        void trigger_runtime_error(const String& message) override;

        /**
         * @brief Get current call frame (for vararg access)
         */
        [[nodiscard]] const CallFrame* current_call_frame() const noexcept;

        /**
         * @brief Set stack top position (for multi-return value handling)
         */
        void set_stack_top(Size new_top) noexcept;

    private:
        VMConfig config_;
        VMState state_ = VMState::Ready;

        // Memory management
        std::unique_ptr<RuntimeMemoryManager> owned_memory_manager_;
        RuntimeMemoryManager* memory_manager_ = nullptr;

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
        Value error_obj_{};  // Stores the current error object

        // Instruction execution methods
        Status execute_instruction(OpCode opcode, Instruction instruction);

        // Stack operations
        void ensure_stack_size(Size size);

        // Function call implementations
        Result<std::vector<Value>> call_lua_function(GCPtr<Function> function,
                                                     const std::vector<Value>& args);

        // Error handling and stack unwinding
        void unwind_stack_to_protected_call();
        [[nodiscard]] String generate_stack_trace_string() const;
        Status call_c_function_protected(const Value& func,
                                     const std::vector<Value>& args,
                                     std::vector<Value>& results);

        // Legacy call operations (for backward compatibility)
        Status call_function(const Value& function, Size arg_count, Size result_count);

        // Legacy instruction implementations (deprecated - use strategy pattern instead)

        // Upvalue management
        class Upvalue* find_upvalue(Value* stack_location);
        void close_upvalues(Value* level);

        // Helper methods
        Value constant_to_value(const backend::ConstantValue& constant);

        // Special function handling
        [[nodiscard]] bool is_tostring_function(const GCPtr<Function>& function) const;
        std::vector<Value> call_tostring_with_metamethod(const std::vector<Value>& args);
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
        VMState saved_state_{};
        std::vector<Value> saved_stack_;
        std::vector<CallFrame> saved_call_stack_;
        Size saved_stack_top_ = 0;
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
