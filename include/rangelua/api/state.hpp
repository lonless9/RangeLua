#pragma once

/**
 * @file state.hpp
 * @brief Enhanced Lua state management API with comprehensive features
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../runtime/value.hpp"
#include "../runtime/vm.hpp"

namespace rangelua::api {

    /**
     * @brief Configuration for State initialization
     */
    struct StateConfig {
        runtime::VMConfig vm_config{};
        bool enable_debug = false;
        bool enable_profiling = false;
        Size initial_stack_size = 1024;
        Size max_stack_size = 65536;
    };

    /**
     * @brief Enhanced Lua state class with comprehensive API support
     */
    class State : public runtime::IVMContext {
    public:
        explicit State();
        explicit State(const StateConfig& config);
        ~State();

        // Non-copyable, movable
        State(const State&) = delete;
        State& operator=(const State&) = delete;
        State(State&&) noexcept = default;
        State& operator=(State&&) noexcept = default;

        /**
         * @brief Execute Lua code
         */
        Result<std::vector<runtime::Value>> execute(StringView code, String name = "<input>");

        /**
         * @brief Load and execute file
         */
        Result<std::vector<runtime::Value>> execute_file(const String& filename);

        /**
         * @brief Get stack size
         */
        [[nodiscard]] Size stack_size() const noexcept override;

        /**
         * @brief Push value onto stack
         */
        void push(runtime::Value value) override;

        /**
         * @brief Pop value from stack
         */
        runtime::Value pop() override;

        /**
         * @brief Get top of stack
         */
        [[nodiscard]] const runtime::Value& top() const override;

        /**
         * @brief Get value at stack index
         */
        [[nodiscard]] const runtime::Value& get(Size index) const;

        /**
         * @brief Set value at stack index
         */
        void set(Size index, runtime::Value value);

        /**
         * @brief Get global variable
         */
        [[nodiscard]] runtime::Value get_global(const String& name) const override;

        /**
         * @brief Set global variable
         */
        void set_global(const String& name, runtime::Value value) override;

        // IVMContext implementation
        [[nodiscard]] runtime::Value& stack_at(Register reg) override;
        [[nodiscard]] const runtime::Value& stack_at(Register reg) const override;
        [[nodiscard]] Size instruction_pointer() const noexcept override;
        void set_instruction_pointer(Size ip) noexcept override;
        void adjust_instruction_pointer(std::int32_t offset) noexcept override;
        [[nodiscard]] const backend::BytecodeFunction* current_function() const noexcept override;
        [[nodiscard]] Size call_depth() const noexcept override;
        [[nodiscard]] runtime::Value get_constant(std::uint16_t index) const override;
        Status call_function(const runtime::Value& function,
                             const std::vector<runtime::Value>& args,
                             std::vector<runtime::Value>& results) override;
        Result<std::vector<runtime::Value>> pcall(const runtime::Value& function,
                                                  const std::vector<runtime::Value>& args) override;
        Result<std::vector<runtime::Value>>
        xpcall(const runtime::Value& function,
               const runtime::Value& msgh,
               const std::vector<runtime::Value>& args) override;
        Status setup_call_frame(const backend::BytecodeFunction& function, Size arg_count) override;
        Status return_from_function(Size result_count) override;
        void set_error(ErrorCode code) override;
        void set_runtime_error(const String& message) override;
        void trigger_runtime_error(const String& message) override;
        runtime::RuntimeMemoryManager& memory_manager() noexcept override;
        [[nodiscard]] runtime::Value get_upvalue(UpvalueIndex index) const override;
        void set_upvalue(UpvalueIndex index, const runtime::Value& value) override;

        /**
         * @brief Check if global variable exists
         */
        [[nodiscard]] bool has_global(const String& name) const;

        /**
         * @brief Clear all global variables
         */
        void clear_globals();

        /**
         * @brief Get VM state
         */
        [[nodiscard]] runtime::VMState vm_state() const noexcept;

        /**
         * @brief Reset state to initial condition
         */
        void reset();

        /**
         * @brief Get configuration
         */
        [[nodiscard]] const StateConfig& config() const noexcept { return config_; }

        runtime::VirtualMachine& get_vm() override { return *vm_; }

    private:
        std::unique_ptr<runtime::VirtualMachine> vm_;
        StateConfig config_;

        /**
         * @brief Initialize global environment
         */
        void initialize_globals();

        /**
         * @brief Setup standard library functions
         */
        void setup_standard_library();

        /**
         * @brief Cleanup state resources and break circular references
         */
        void cleanup();
    };

}  // namespace rangelua::api
