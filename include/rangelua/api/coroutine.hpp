#pragma once

/**
 * @file coroutine.hpp
 * @brief Coroutine API
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

/**
 * @brief Coroutine wrapper
 */
class Coroutine {
public:
    explicit Coroutine(runtime::Value value);

    /**
     * @brief Resume coroutine
     */
    Result<std::vector<runtime::Value>> resume(const std::vector<runtime::Value>& args = {});

    /**
     * @brief Check if coroutine is suspended
     */
    [[nodiscard]] bool is_suspended() const noexcept;

    /**
     * @brief Check if coroutine is dead
     */
    [[nodiscard]] bool is_dead() const noexcept;

private:
    runtime::Value coroutine_;
};

} // namespace rangelua::api
