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
        Size instruction_pointer = 0;
        Size stack_base = 0;
        Size local_count = 0;
        bool is_tail_call = false;

        CallFrame() = default;
        CallFrame(const backend::BytecodeFunction* func, Size stack_base, Size locals)
            : function(func), stack_base(stack_base), local_count(locals) {}
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
        std::unordered_map<String, Value> globals_;

        // Upvalue management
        class Upvalue* open_upvalues_ = nullptr;  // Linked list of open upvalues

        // Current execution context
        Size stack_top_ = 0;
        ErrorCode last_error_ = ErrorCode::SUCCESS;

        // Instruction execution methods
        Status execute_instruction(OpCode opcode, Instruction instruction);

        // Stack operations
        void ensure_stack_size(Size size);

        // Legacy call operations (for backward compatibility)
        Status call_function(const Value& function, Size arg_count, Size result_count);

        // Instruction implementations
        Status op_move(Register a, Register b);
        Status op_loadi(Register a, std::int16_t sbx);
        Status op_loadf(Register a, std::int16_t sbx);
        Status op_loadk(Register a, std::uint16_t bx);
        Status op_loadnil(Register a, Register b);
        Status op_loadtrue(Register a);
        Status op_loadfalse(Register a);

        Status op_add(Register a, Register b, Register c);
        Status op_sub(Register a, Register b, Register c);
        Status op_mul(Register a, Register b, Register c);
        Status op_div(Register a, Register b, Register c);
        Status op_mod(Register a, Register b, Register c);
        Status op_pow(Register a, Register b, Register c);
        Status op_unm(Register a, Register b);

        Status op_band(Register a, Register b, Register c);
        Status op_bor(Register a, Register b, Register c);
        Status op_bxor(Register a, Register b, Register c);
        Status op_shl(Register a, Register b, Register c);
        Status op_shr(Register a, Register b, Register c);
        Status op_bnot(Register a, Register b);

        Status op_eq(Register a, Register b, std::int16_t sbx);
        Status op_lt(Register a, Register b, std::int16_t sbx);
        Status op_le(Register a, Register b, std::int16_t sbx);

        Status op_not(Register a, Register b);
        Status op_len(Register a, Register b);

        Status op_gettable(Register a, Register b, Register c);
        Status op_gettabup(Register a, Register b, Register c);
        Status op_settable(Register a, Register b, Register c);
        Status op_newtable(Register a);

        Status op_call(Register a, Register b, Register c);
        Status op_tailcall(Register a, Register b);
        Status op_return(Register a, Register b);

        Status op_jmp(std::int16_t sbx);
        Status op_test(Register a, Register c);
        Status op_testset(Register a, Register b, Register c);

        Status op_getupval(Register a, UpvalueIndex b);
        Status op_setupval(Register a, UpvalueIndex b);

        Status op_getglobal(Register a, std::uint16_t bx);
        Status op_setglobal(Register a, std::uint16_t bx);

        Status op_closure(Register a, std::uint16_t bx);

        // Additional table operations
        Status op_geti(Register a, Register b, Register c);
        Status op_getfield(Register a, Register b, Register c);
        Status op_settabup(Register a, Register b, Register c);
        Status op_seti(Register a, Register b, Register c);
        Status op_setfield(Register a, Register b, Register c);
        Status op_self(Register a, Register b, Register c);

        // Arithmetic with constants
        Status op_addi(Register a, Register b, std::int8_t c);
        Status op_addk(Register a, Register b, Register c);
        Status op_subk(Register a, Register b, Register c);
        Status op_mulk(Register a, Register b, Register c);
        Status op_modk(Register a, Register b, Register c);
        Status op_powk(Register a, Register b, Register c);
        Status op_divk(Register a, Register b, Register c);
        Status op_idivk(Register a, Register b, Register c);

        // Bitwise with constants
        Status op_bandk(Register a, Register b, Register c);
        Status op_bork(Register a, Register b, Register c);
        Status op_bxork(Register a, Register b, Register c);
        Status op_shri(Register a, Register b, std::int8_t c);
        Status op_shli(Register a, Register b, std::int8_t c);

        // Additional arithmetic operations
        Status op_idiv(Register a, Register b, Register c);

        // Metamethod operations
        Status op_mmbin(Register a, Register b, Register c);
        Status op_mmbini(Register a, Register b, std::int16_t sbx);
        Status op_mmbink(Register a, Register b, Register c);

        // String operations
        Status op_concat(Register a, Register b);

        // Control flow
        Status op_close(Register a);
        Status op_tbc(Register a);

        // Additional comparison operations
        Status op_eqk(Register a, Register b, std::int16_t sbx);
        Status op_eqi(Register a, Register b, std::int16_t sbx);
        Status op_lti(Register a, Register b, std::int16_t sbx);
        Status op_lei(Register a, Register b, std::int16_t sbx);
        Status op_gti(Register a, Register b, std::int16_t sbx);
        Status op_gei(Register a, Register b, std::int16_t sbx);

        // Additional function operations
        Status op_return0();
        Status op_return1(Register a);

        // Loop operations
        Status op_forloop(Register a, std::int16_t bx);
        Status op_forprep(Register a, std::int16_t bx);
        Status op_tforprep(Register a, std::int16_t bx);
        Status op_tforcall(Register a, Register c);
        Status op_tforloop(Register a, std::int16_t bx);

        // List operations
        Status op_setlist(Register a, Register b, Register c);

        // Vararg operations
        Status op_vararg(Register a, Register c);
        Status op_varargprep(Register a, Register b);

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
