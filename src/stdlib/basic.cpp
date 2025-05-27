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

    void register_functions(const runtime::GCPtr<runtime::Table>& globals) {
        // Register the print function
        globals->set(runtime::Value("print"), runtime::value_factory::function(print));
        globals->set(runtime::Value("type"), runtime::value_factory::function(type));
    }

}  // namespace rangelua::stdlib::basic
