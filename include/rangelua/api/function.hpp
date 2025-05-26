#pragma once

/**
 * @file function.hpp
 * @brief Function API
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

/**
 * @brief Function wrapper
 */
class Function {
public:
    explicit Function(runtime::Value value);

    /**
     * @brief Call function with arguments
     */
    Result<std::vector<runtime::Value>> call(const std::vector<runtime::Value>& args);

    /**
     * @brief Get function arity
     */
    [[nodiscard]] Size arity() const noexcept;

private:
    runtime::Value function_;
};

} // namespace rangelua::api
