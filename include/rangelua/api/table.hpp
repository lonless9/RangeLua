#pragma once

/**
 * @file table.hpp
 * @brief Table API
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

    /**
     * @brief Table wrapper
     */
    class Table {
    public:
        explicit Table(runtime::Value value);

        /**
         * @brief Get value by key
         */
        [[nodiscard]] runtime::Value get(const runtime::Value& key) const;

        /**
         * @brief Set value by key
         */
        void set(const runtime::Value& key, const runtime::Value& value);

        /**
         * @brief Get table size
         */
        [[nodiscard]] Size size() const noexcept;

    private:
        runtime::Value table_;
    };

}  // namespace rangelua::api
