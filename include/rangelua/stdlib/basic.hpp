#pragma once

/**
 * @file basic.hpp
 * @brief Lua basic library implementation for RangeLua
 * @version 0.1.0
 *
 * Implements the basic Lua library functions including print, type, tostring,
 * tonumber, and other fundamental functions that are available in the global
 * environment by default.
 */

#include <vector>

#include "../core/types.hpp"
#include "../runtime/value.hpp"
#include "../runtime/vm/instruction_strategy.hpp"

namespace rangelua::stdlib {

    /**
     * @brief Basic library function registry
     *
     * Contains all the basic library functions that should be available
     * in the global environment by default.
     */
    namespace basic {

        /**
         * @brief Lua print function implementation
         *
         * Prints the given arguments to stdout, separated by tabs and followed
         * by a newline. Converts all arguments to strings using tostring semantics.
         *
         * @param args Vector of arguments to print
         * @return Empty vector (print returns no values)
         */
        std::vector<runtime::Value> print(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua type function implementation
         *
         * Returns the type of the given value as a string.
         *
         * @param args Vector containing the value to check (only first argument used)
         * @return Vector containing the type name as a string
         */
        std::vector<runtime::Value> type(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua ipairs function implementation
         *
         * Returns an iterator function, the table, and initial index (0) for
         * iterating over array-like tables in numerical order.
         *
         * @param args Vector containing the table to iterate (only first argument used)
         * @return Vector containing iterator function, table, and initial index
         */
        std::vector<runtime::Value> ipairs(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua pairs function implementation
         *
         * Returns an iterator function, the table, and nil for iterating over
         * all key-value pairs in a table.
         *
         * @param args Vector containing the table to iterate (only first argument used)
         * @return Vector containing iterator function, table, and nil
         */
        std::vector<runtime::Value> pairs(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Iterator function for ipairs
         *
         * Internal iterator function used by ipairs to traverse array-like tables.
         *
         * @param args Vector containing table and current index
         * @return Vector containing next index and value, or just index if end reached
         */
        std::vector<runtime::Value> ipairsaux(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Iterator function for pairs
         *
         * Internal iterator function used by pairs to traverse all table entries.
         *
         * @param args Vector containing table and current key
         * @return Vector containing next key and value, or empty if end reached
         */
        std::vector<runtime::Value> next(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua tostring function implementation
         *
         * Converts the given value to a string representation.
         *
         * @param args Vector containing the value to convert
         * @return Vector containing the string representation
         */
        std::vector<runtime::Value> tostring(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua tonumber function implementation
         *
         * Converts the given value to a number.
         *
         * @param args Vector containing the value to convert and optional base
         * @return Vector containing the number or nil if conversion failed
         */
        std::vector<runtime::Value> tonumber(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua getmetatable function implementation
         *
         * Returns the metatable of the given value.
         *
         * @param args Vector containing the value
         * @return Vector containing the metatable or nil
         */
        std::vector<runtime::Value> getmetatable(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua setmetatable function implementation
         *
         * Sets the metatable of the given table.
         *
         * @param args Vector containing the table and metatable
         * @return Vector containing the table
         */
        std::vector<runtime::Value> setmetatable(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua rawget function implementation
         *
         * Gets a table element without invoking metamethods.
         *
         * @param args Vector containing the table and key
         * @return Vector containing the value
         */
        std::vector<runtime::Value> rawget(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua rawset function implementation
         *
         * Sets a table element without invoking metamethods.
         *
         * @param args Vector containing the table, key, and value
         * @return Vector containing the table
         */
        std::vector<runtime::Value> rawset(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua rawequal function implementation
         *
         * Checks equality without invoking metamethods.
         *
         * @param args Vector containing two values to compare
         * @return Vector containing boolean result
         */
        std::vector<runtime::Value> rawequal(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua rawlen function implementation
         *
         * Gets length without invoking metamethods.
         *
         * @param args Vector containing the value
         * @return Vector containing the length
         */
        std::vector<runtime::Value> rawlen(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua select function implementation
         *
         * Returns selected arguments from a list.
         *
         * @param args Vector containing index and arguments
         * @return Vector containing selected arguments
         */
        std::vector<runtime::Value> select(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua error function implementation
         *
         * Raises an error with the given message.
         *
         * @param args Vector containing error message and optional level
         * @return Never returns (throws exception)
         */
        std::vector<runtime::Value> error(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua assert function implementation
         *
         * Asserts that a condition is true.
         *
         * @param args Vector containing condition and optional message
         * @return Vector containing the first argument if true
         */
        std::vector<runtime::Value> assert_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua pcall function implementation
         *
         * Calls a function in protected mode.
         *
         * @param args Vector containing the function and its arguments
         * @return Vector containing success status and results or error message
         */
        std::vector<runtime::Value> pcall_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Lua xpcall function implementation
         *
         * Calls a function in protected mode with a message handler.
         *
         * @param args Vector containing the function, message handler, and arguments
         * @return Vector containing success status and results or error message
         */
        std::vector<runtime::Value> xpcall_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

        /**
         * @brief Register basic library functions in the given global table
         *
         * This function registers the implemented basic library functions in the provided
         * global environment table, making them available for Lua code.
         *
         * @param globals The global environment table to register functions in
         */
        void register_functions(runtime::IVMContext* vm, const runtime::GCPtr<runtime::Table>& globals);

    }  // namespace basic

}  // namespace rangelua::stdlib
