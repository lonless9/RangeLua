#pragma once

/**
 * @file environment.hpp
 * @brief Environment and global table management for RangeLua
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "value.hpp"
#include "objects.hpp"

namespace rangelua::runtime {

    /**
     * @brief Environment management for Lua _ENV semantics
     *
     * Manages the _ENV environment table and global variable access
     * following Lua 5.5 semantics. Provides proper environment
     * inheritance and scoping rules.
     */
    class Environment {
    public:
        /**
         * @brief Create environment with global table
         * @param global_table The global table to use as _ENV
         */
        explicit Environment(GCPtr<Table> global_table);

        /**
         * @brief Create environment inheriting from parent
         * @param parent_env Parent environment
         * @param env_table Environment table (defaults to parent's if null)
         */
        Environment(const Environment& parent_env, GCPtr<Table> env_table = GCPtr<Table>{});

        ~Environment();

        // Non-copyable but movable
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;
        Environment(Environment&&) noexcept = default;
        Environment& operator=(Environment&&) noexcept = default;

        /**
         * @brief Get the _ENV table
         */
        [[nodiscard]] GCPtr<Table> getEnvTable() const noexcept;

        /**
         * @brief Set the _ENV table
         */
        void setEnvTable(GCPtr<Table> env_table) noexcept;

        /**
         * @brief Get global variable value
         * @param name Variable name
         * @return Variable value or nil if not found
         */
        [[nodiscard]] Value getGlobal(const String& name) const;

        /**
         * @brief Set global variable value
         * @param name Variable name
         * @param value Variable value
         */
        void setGlobal(const String& name, const Value& value);

        /**
         * @brief Check if global variable exists
         * @param name Variable name
         * @return True if variable exists and is not nil
         */
        [[nodiscard]] bool hasGlobal(const String& name) const;

        /**
         * @brief Get the global table (root environment)
         */
        [[nodiscard]] GCPtr<Table> getGlobalTable() const noexcept;

    private:
        GCPtr<Table> env_table_;      // Current _ENV table
        GCPtr<Table> global_table_;   // Root global table
    };

    /**
     * @brief Registry management for global state
     *
     * Manages the Lua registry similar to Lua 5.5, providing
     * storage for global table and other system values.
     */
    class Registry {
    public:
        // Registry indices (following Lua 5.5 conventions)
        static constexpr Size RIDX_MAINTHREAD = 1;
        static constexpr Size RIDX_GLOBALS = 2;
        static constexpr Size RIDX_LAST = RIDX_GLOBALS;

        /**
         * @brief Create registry with initial values
         */
        Registry();

        ~Registry();

        // Non-copyable but movable
        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;
        Registry(Registry&&) noexcept = default;
        Registry& operator=(Registry&&) noexcept = default;

        /**
         * @brief Get registry table
         */
        [[nodiscard]] GCPtr<Table> getRegistryTable() const noexcept;

        /**
         * @brief Get global table from registry
         */
        [[nodiscard]] GCPtr<Table> getGlobalTable() const;

        /**
         * @brief Set global table in registry
         */
        void setGlobalTable(GCPtr<Table> global_table);

        /**
         * @brief Get value from registry by index
         */
        [[nodiscard]] Value getRegistryValue(Size index) const;

        /**
         * @brief Set value in registry by index
         */
        void setRegistryValue(Size index, const Value& value);

    private:
        GCPtr<Table> registry_table_;
    };

}  // namespace rangelua::runtime
