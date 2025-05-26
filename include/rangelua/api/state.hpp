#pragma once

/**
 * @file state.hpp
 * @brief Lua state management API
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../runtime/vm.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

/**
 * @brief Main Lua state class
 */
class State {
public:
    explicit State();
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

private:
    runtime::VirtualMachine vm_;
};

} // namespace rangelua::api
