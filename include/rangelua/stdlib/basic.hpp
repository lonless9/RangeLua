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
        std::vector<runtime::Value> print(const std::vector<runtime::Value>& args);

        /**
         * @brief Lua type function implementation
         *
         * Returns the type of the given value as a string.
         *
         * @param args Vector containing the value to check (only first argument used)
         * @return Vector containing the type name as a string
         */
        std::vector<runtime::Value> type(const std::vector<runtime::Value>& args);

        /**
         * @brief Lua ipairs function implementation
         *
         * Returns an iterator function, the table, and initial index (0) for
         * iterating over array-like tables in numerical order.
         *
         * @param args Vector containing the table to iterate (only first argument used)
         * @return Vector containing iterator function, table, and initial index
         */
        std::vector<runtime::Value> ipairs(const std::vector<runtime::Value>& args);

        /**
         * @brief Lua pairs function implementation
         *
         * Returns an iterator function, the table, and nil for iterating over
         * all key-value pairs in a table.
         *
         * @param args Vector containing the table to iterate (only first argument used)
         * @return Vector containing iterator function, table, and nil
         */
        std::vector<runtime::Value> pairs(const std::vector<runtime::Value>& args);

        /**
         * @brief Iterator function for ipairs
         *
         * Internal iterator function used by ipairs to traverse array-like tables.
         *
         * @param args Vector containing table and current index
         * @return Vector containing next index and value, or just index if end reached
         */
        std::vector<runtime::Value> ipairsaux(const std::vector<runtime::Value>& args);

        /**
         * @brief Iterator function for pairs
         *
         * Internal iterator function used by pairs to traverse all table entries.
         *
         * @param args Vector containing table and current key
         * @return Vector containing next key and value, or empty if end reached
         */
        std::vector<runtime::Value> next(const std::vector<runtime::Value>& args);

        /**
         * @brief Register basic library functions in the given global table
         *
         * This function registers the implemented basic library functions in the provided
         * global environment table, making them available for Lua code.
         *
         * @param globals The global environment table to register functions in
         */
        void register_functions(const runtime::GCPtr<runtime::Table>& globals);

    }  // namespace basic

}  // namespace rangelua::stdlib
