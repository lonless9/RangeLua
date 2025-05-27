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
