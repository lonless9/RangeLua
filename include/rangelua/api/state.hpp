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
    class State {
    public:
        explicit State();
        explicit State(const StateConfig& config);
        ~State() = default;

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
        Size stack_size() const noexcept;

        /**
         * @brief Push value onto stack
         */
        void push(runtime::Value value);

        /**
         * @brief Pop value from stack
         */
        runtime::Value pop();

        /**
         * @brief Get top of stack
         */
        const runtime::Value& top() const;

        /**
         * @brief Get value at stack index
         */
        const runtime::Value& get(Size index) const;

        /**
         * @brief Set value at stack index
         */
        void set(Size index, runtime::Value value);

        /**
         * @brief Get global variable
         */
        runtime::Value get_global(const String& name) const;

        /**
         * @brief Set global variable
         */
        void set_global(const String& name, runtime::Value value);

        /**
         * @brief Check if global variable exists
         */
        bool has_global(const String& name) const;

        /**
         * @brief Clear all global variables
         */
        void clear_globals();

        /**
         * @brief Get VM state
         */
        runtime::VMState vm_state() const noexcept;

        /**
         * @brief Reset state to initial condition
         */
        void reset();

        /**
         * @brief Get configuration
         */
        const StateConfig& config() const noexcept { return config_; }

    private:
        runtime::VirtualMachine vm_;
        StateConfig config_;

        /**
         * @brief Initialize global environment
         */
        void initialize_globals();

        /**
         * @brief Setup standard library functions
         */
        void setup_standard_library();
    };

}  // namespace rangelua::api
