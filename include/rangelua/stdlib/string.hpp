#pragma once

/**
 * @file string.hpp
 * @brief Lua string library implementation for RangeLua
 * @version 0.1.0
 *
 * Implements the Lua string library functions including string manipulation,
 * pattern matching, formatting, and other string operations.
 */

#include <vector>

#include "../runtime/value.hpp"
#include "../runtime/vm/instruction_strategy.hpp"

namespace rangelua::stdlib::string {

    /**
     * @brief Lua string.byte function implementation
     *
     * Returns the internal numeric codes of the characters in a string.
     *
     * @param args Vector containing string and optional start/end positions
     * @return Vector containing numeric codes
     */
    std::vector<runtime::Value> byte(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.char function implementation
     *
     * Returns a string with characters having the given numeric codes.
     *
     * @param args Vector containing numeric codes
     * @return Vector containing the resulting string
     */
    std::vector<runtime::Value> char_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.find function implementation
     *
     * Looks for the first match of pattern in the string.
     *
     * @param args Vector containing string, pattern, and optional start position
     * @return Vector containing start and end positions or nil
     */
    std::vector<runtime::Value> find(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.format function implementation
     *
     * Returns a formatted version of its variable number of arguments.
     *
     * @param args Vector containing format string and arguments
     * @return Vector containing formatted string
     */
    std::vector<runtime::Value> format(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.gsub function implementation
     *
     * Returns a copy of string with all occurrences of pattern replaced.
     *
     * @param args Vector containing string, pattern, replacement, and optional count
     * @return Vector containing modified string and number of substitutions
     */
    std::vector<runtime::Value> gsub(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.len function implementation
     *
     * Returns the length of the string.
     *
     * @param args Vector containing the string
     * @return Vector containing the length
     */
    std::vector<runtime::Value> len(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.lower function implementation
     *
     * Returns a copy of the string with all uppercase letters changed to lowercase.
     *
     * @param args Vector containing the string
     * @return Vector containing the lowercase string
     */
    std::vector<runtime::Value> lower(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.match function implementation
     *
     * Looks for the first match of pattern in the string.
     *
     * @param args Vector containing string, pattern, and optional start position
     * @return Vector containing captured values or nil
     */
    std::vector<runtime::Value> match(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.rep function implementation
     *
     * Returns a string that is the concatenation of n copies of the string.
     *
     * @param args Vector containing string, count, and optional separator
     * @return Vector containing repeated string
     */
    std::vector<runtime::Value> rep(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.reverse function implementation
     *
     * Returns a string that is the string reversed.
     *
     * @param args Vector containing the string
     * @return Vector containing reversed string
     */
    std::vector<runtime::Value> reverse(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.sub function implementation
     *
     * Returns the substring of the string that starts at i and continues until j.
     *
     * @param args Vector containing string, start, and optional end position
     * @return Vector containing substring
     */
    std::vector<runtime::Value> sub(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Lua string.upper function implementation
     *
     * Returns a copy of the string with all lowercase letters changed to uppercase.
     *
     * @param args Vector containing the string
     * @return Vector containing the uppercase string
     */
    std::vector<runtime::Value> upper(runtime::IVMContext* vm, const std::vector<runtime::Value>& args);

    /**
     * @brief Register string library functions in the given global table
     *
     * This function creates the string table and registers all string library
     * functions in it, then adds it to the global environment.
     *
     * @param globals The global environment table to register the string library in
     */
    void register_functions(runtime::IVMContext* vm, const runtime::GCPtr<runtime::Table>& globals);

}  // namespace rangelua::stdlib::string
