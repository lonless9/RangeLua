/**
 * @file basic.cpp
 * @brief Implementation of Lua basic library functions for RangeLua
 * @version 0.1.0
 */

#include <rangelua/stdlib/basic.hpp>

#include <iostream>

#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>

namespace rangelua::stdlib::basic {

    std::vector<runtime::Value> print(const std::vector<runtime::Value>& args) {
        // Convert all arguments to strings and print them
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) {
                std::cout << "\t";  // Tab separator between arguments
            }

            // Convert value to string using Lua semantics
            std::string str;
            if (args[i].is_nil()) {
                str = "nil";
            } else if (args[i].is_boolean()) {
                auto bool_result = args[i].to_boolean();
                if (std::holds_alternative<bool>(bool_result)) {
                    str = std::get<bool>(bool_result) ? "true" : "false";
                } else {
                    str = "nil";  // Fallback
                }
            } else if (args[i].is_number()) {
                auto num_result = args[i].to_number();
                if (std::holds_alternative<double>(num_result)) {
                    double num = std::get<double>(num_result);
                    // Format number similar to Lua's default formatting
                    if (num == static_cast<double>(static_cast<Int>(num))) {
                        str = std::to_string(static_cast<Int>(num));
                    } else {
                        str = std::to_string(num);
                        // Remove trailing zeros
                        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                        str.erase(str.find_last_not_of('.') + 1, std::string::npos);
                    }
                } else {
                    str = "nil";  // Fallback
                }
            } else if (args[i].is_string()) {
                auto str_result = args[i].to_string();
                if (std::holds_alternative<std::string>(str_result)) {
                    str = std::get<std::string>(str_result);
                } else {
                    str = "nil";  // Fallback
                }
            } else {
                // For other types (table, function, userdata, thread), use type name
                str = args[i].type_name();
            }

            std::cout << str;
        }

        std::cout << '\n';  // Newline at the end
        return {};  // print returns no values
    }

    std::vector<runtime::Value> type(const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value("nil")};
        }

        // Return the type name of the first argument
        std::string type_name = args[0].type_name();
        return {runtime::Value(type_name)};
    }

    std::vector<runtime::Value> ipairsaux(const std::vector<runtime::Value>& args) {
        if (args.size() < 2) {
            return {runtime::Value{}};  // Return nil to signal end of iteration
        }

        // Get the table and current index
        const auto& table_value = args[0];
        const auto& index_value = args[1];

        if (!table_value.is_table()) {
            return {runtime::Value{}};  // Return nil to signal end of iteration
        }

        // Convert index to integer and increment
        auto index_result = index_value.to_number();
        if (!std::holds_alternative<double>(index_result)) {
            return {runtime::Value{}};  // Return nil to signal end of iteration
        }

        Int current_index = static_cast<Int>(std::get<double>(index_result));
        Int next_index = current_index + 1;

        // Get the table
        auto table_result = table_value.to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {runtime::Value{}};  // Return nil to signal end of iteration
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);

        // Get the value at the next index
        runtime::Value key(static_cast<Number>(next_index));
        runtime::Value value = table->get(key);

        if (value.is_nil()) {
            // End of iteration - return nil to signal end of iteration
            return {runtime::Value{}};
        }

        // Return index and value
        return {runtime::Value(static_cast<Number>(next_index)), value};
    }

    std::vector<runtime::Value> ipairs(const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value(), runtime::Value(), runtime::Value()};  // Return nil values
        }

        const auto& table_value = args[0];

        // Return iterator function, table, and initial index (0)
        return {runtime::value_factory::function(ipairsaux),
                table_value,
                runtime::Value(static_cast<Number>(0))};
    }

    std::vector<runtime::Value> next(const std::vector<runtime::Value>& args) {
        if (args.size() < 2) {
            return {};  // Return empty if insufficient arguments
        }

        const auto& table_value = args[0];
        const auto& key_value = args[1];

        if (!table_value.is_table()) {
            return {};  // Return empty if not a table
        }

        auto table_result = table_value.to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {};  // Return empty if table conversion failed
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);

        // If key is nil, return the first key-value pair
        if (key_value.is_nil()) {
            // Get the first element using iterator
            auto it = table->begin();
            if (it != table->end()) {
                auto [key, value] = *it;
                return {key, value};
            }
            return {};  // Empty table
        }

        // Find the current key and return the next one
        bool found_current = false;
        for (const auto& [current_key, current_value] : *table) {
            if (found_current) {
                // Return the next key-value pair
                return {current_key, current_value};
            }

            // Check if this is the key we're looking for
            if (current_key == key_value) {
                found_current = true;
            }
        }

        // End of iteration - key not found or was the last key
        return {};
    }

    std::vector<runtime::Value> pairs(const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value(), runtime::Value(), runtime::Value()};  // Return nil values
        }

        const auto& table_value = args[0];

        // Return iterator function, table, and nil
        return {
            runtime::value_factory::function(next),
            table_value,
            runtime::Value()  // nil
        };
    }

    void register_functions(const runtime::GCPtr<runtime::Table>& globals) {
        // Register the basic library functions
        globals->set(runtime::Value("print"), runtime::value_factory::function(print));
        globals->set(runtime::Value("type"), runtime::value_factory::function(type));
        globals->set(runtime::Value("ipairs"), runtime::value_factory::function(ipairs));
        globals->set(runtime::Value("pairs"), runtime::value_factory::function(pairs));
        globals->set(runtime::Value("next"), runtime::value_factory::function(next));
    }

}  // namespace rangelua::stdlib::basic
