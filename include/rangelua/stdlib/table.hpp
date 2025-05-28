#pragma once

/**
 * @file table.hpp
 * @brief Lua table library implementation for RangeLua
 * @version 0.1.0
 *
 * Implements the Lua table library functions including table manipulation,
 * concatenation, insertion, removal, and sorting operations.
 */

#include <vector>

#include "../runtime/value.hpp"

namespace rangelua::stdlib::table {

    /**
     * @brief Lua table.concat function implementation
     *
     * Concatenates the elements of a table into a string.
     *
     * @param args Vector containing table, optional separator, start, and end
     * @return Vector containing the concatenated string
     */
    std::vector<runtime::Value> concat(const std::vector<runtime::Value>& args);

    /**
     * @brief Lua table.insert function implementation
     *
     * Inserts element into a table at the specified position.
     *
     * @param args Vector containing table, optional position, and value
     * @return Vector (empty)
     */
    std::vector<runtime::Value> insert(const std::vector<runtime::Value>& args);

    /**
     * @brief Lua table.move function implementation
     *
     * Moves elements from one table to another.
     *
     * @param args Vector containing source table, start, end, destination start, and optional destination table
     * @return Vector containing the destination table
     */
    std::vector<runtime::Value> move(const std::vector<runtime::Value>& args);

    /**
     * @brief Lua table.pack function implementation
     *
     * Packs the given arguments into a table with field "n" set to the number of arguments.
     *
     * @param args Vector containing the arguments to pack
     * @return Vector containing the packed table
     */
    std::vector<runtime::Value> pack(const std::vector<runtime::Value>& args);

    /**
     * @brief Lua table.remove function implementation
     *
     * Removes element from a table at the specified position.
     *
     * @param args Vector containing table and optional position
     * @return Vector containing the removed element
     */
    std::vector<runtime::Value> remove(const std::vector<runtime::Value>& args);

    /**
     * @brief Lua table.sort function implementation
     *
     * Sorts table elements in place.
     *
     * @param args Vector containing table and optional comparison function
     * @return Vector (empty)
     */
    std::vector<runtime::Value> sort(const std::vector<runtime::Value>& args);

    /**
     * @brief Lua table.unpack function implementation
     *
     * Unpacks elements from a table.
     *
     * @param args Vector containing table, optional start, and end positions
     * @return Vector containing the unpacked elements
     */
    std::vector<runtime::Value> unpack(const std::vector<runtime::Value>& args);

    /**
     * @brief Register table library functions in the given global table
     *
     * This function creates the table table and registers all table library
     * functions in it, then adds it to the global environment.
     *
     * @param globals The global environment table to register the table library in
     */
    void register_functions(const runtime::GCPtr<runtime::Table>& globals);

}  // namespace rangelua::stdlib::table
