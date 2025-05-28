/**
 * @file environment.cpp
 * @brief Implementation of Environment and Registry classes for RangeLua
 * @version 0.1.0
 */

#include <rangelua/runtime/environment.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // Environment implementation
    Environment::Environment(GCPtr<Table> global_table)
        : env_table_(global_table), global_table_(global_table) {
        if (!global_table_) {
            // Create a new global table if none provided
            global_table_ = makeGCObject<Table>();
            env_table_ = global_table_;
        }
    }

    Environment::Environment(const Environment& parent_env, GCPtr<Table> env_table)
        : env_table_(env_table.get() ? env_table : parent_env.env_table_),
          global_table_(parent_env.global_table_) {
        // If no specific env_table provided, inherit from parent
        if (!env_table_.get()) {
            env_table_ = parent_env.env_table_;
        }
    }

    Environment::~Environment() {
        // Explicitly reset GCPtr references to ensure proper cleanup
        env_table_.reset();
        global_table_.reset();
    }

    GCPtr<Table> Environment::getEnvTable() const noexcept {
        return env_table_;
    }

    void Environment::setEnvTable(GCPtr<Table> env_table) noexcept {
        env_table_ = env_table;
    }

    Value Environment::getGlobal(const String& name) const {
        if (!env_table_) {
            return Value{};  // Return nil if no environment table
        }

        Value key(name);
        Value result = env_table_->get(key);

        // Log the global variable access for debugging
        auto logger = utils::Logger::create_logger("environment");
        if (logger) {
            logger->debug("Getting global '{}' = {}", name, result.debug_string());
        }

        return result;
    }

    void Environment::setGlobal(const String& name, const Value& value) {
        if (!env_table_) {
            return;  // Cannot set if no environment table
        }

        Value key(name);
        env_table_->set(key, value);

        // Log the global variable assignment for debugging
        auto logger = utils::Logger::create_logger("environment");
        if (logger) {
            logger->debug("Setting global '{}' = {}", name, value.debug_string());
        }
    }

    bool Environment::hasGlobal(const String& name) const {
        if (!env_table_) {
            return false;
        }

        Value key(name);
        Value result = env_table_->get(key);
        return !result.is_nil();
    }

    GCPtr<Table> Environment::getGlobalTable() const noexcept {
        return global_table_;
    }

    // Registry implementation
    Registry::Registry() {
        // Create the registry table
        registry_table_ = makeGCObject<Table>();

        // Initialize registry with default values
        // registry[1] = false (placeholder for main thread)
        registry_table_->set(Value(static_cast<Number>(RIDX_MAINTHREAD)), Value(false));

        // registry[RIDX_GLOBALS] = new table (global table)
        auto global_table = makeGCObject<Table>();
        registry_table_->set(Value(static_cast<Number>(RIDX_GLOBALS)), Value(global_table));

        auto logger = utils::Logger::create_logger("environment");
        if (logger) {
            logger->debug("Registry initialized with global table");
        }
    }

    Registry::~Registry() {
        // Explicitly reset GCPtr reference to ensure proper cleanup
        registry_table_.reset();
    }

    GCPtr<Table> Registry::getRegistryTable() const noexcept {
        return registry_table_;
    }

    GCPtr<Table> Registry::getGlobalTable() const {
        if (!registry_table_) {
            return GCPtr<Table>{};
        }

        Value key(static_cast<Number>(RIDX_GLOBALS));
        Value global_value = registry_table_->get(key);

        if (global_value.is_table()) {
            auto table_result = global_value.to_table();
            if (std::holds_alternative<GCPtr<Table>>(table_result)) {
                return std::get<GCPtr<Table>>(table_result);
            }
        }

        return GCPtr<Table>{};
    }

    void Registry::setGlobalTable(GCPtr<Table> global_table) {
        if (!registry_table_ || !global_table) {
            return;
        }

        Value key(static_cast<Number>(RIDX_GLOBALS));
        Value value(global_table);
        registry_table_->set(key, value);

        auto logger = utils::Logger::create_logger("environment");
        if (logger) {
            logger->debug("Global table updated in registry");
        }
    }

    Value Registry::getRegistryValue(Size index) const {
        if (!registry_table_) {
            return Value{};
        }

        Value key(static_cast<Number>(index));
        return registry_table_->get(key);
    }

    void Registry::setRegistryValue(Size index, const Value& value) {
        if (!registry_table_) {
            return;
        }

        Value key(static_cast<Number>(index));
        registry_table_->set(key, value);
    }

}  // namespace rangelua::runtime
