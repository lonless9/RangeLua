/**
 * @file basic.cpp
 * @brief Implementation of Lua basic library functions for RangeLua
 * @version 0.1.0
 */

#include <rangelua/core/types.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/stdlib/basic.hpp>

#include <iostream>

namespace rangelua::stdlib::basic {

    std::vector<runtime::Value> print(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        // Convert all arguments to strings and print them
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) {
                std::cout << "\t";  // Tab separator between arguments
            }

            // Convert value to string using Lua semantics, which may involve metamethods
            auto str_result = runtime::Value::tostring_with_metamethod(args[i]);
            std::string str;
            if (std::holds_alternative<std::string>(str_result)) {
                str = std::get<std::string>(str_result);
            } else {
                // Fallback to debug string if metamethod conversion fails
                str = args[i].debug_string();
            }
            std::cout << str;
        }

        std::cout << '\n';  // Newline at the end
        return {};          // print returns no values
    }

    std::vector<runtime::Value> type(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value("nil")};
        }

        // Return the type name of the first argument
        std::string type_name = args[0].type_name();
        return {runtime::Value(type_name)};
    }

    std::vector<runtime::Value> ipairsaux(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
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

    std::vector<runtime::Value> ipairs(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value(), runtime::Value(), runtime::Value()};  // Return nil values
        }

        const auto& table_value = args[0];

        // Return iterator function, table, and initial index (0)
        return {runtime::value_factory::function(ipairsaux, vm),
                table_value,
                runtime::Value(static_cast<Number>(0))};
    }

    std::vector<runtime::Value> next(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
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
                // Continue to next iteration to get the next key-value pair
            }
        }

        // End of iteration - key not found or was the last key
        return {};
    }

    std::vector<runtime::Value> pairs(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value(), runtime::Value(), runtime::Value()};  // Return nil values
        }

        const auto& table_value = args[0];

        // Return iterator function, table, and nil
        return {
            runtime::value_factory::function(next, vm),
            table_value,
            runtime::Value()  // nil
        };
    }

    std::vector<runtime::Value> tostring(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value("nil")};
        }

        const auto& value = args[0];

        // Use the proper tostring conversion that handles metamethods
        auto str_result = runtime::Value::tostring_with_metamethod(value);
        if (std::holds_alternative<std::string>(str_result)) {
            return {runtime::Value(std::get<std::string>(str_result))};
        }

        // Fallback to debug string if metamethod conversion fails
        return {runtime::Value(value.debug_string())};
    }

    std::vector<runtime::Value> tonumber(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value()};  // nil
        }

        const auto& value = args[0];

        if (value.is_number()) {
            return {value};  // Already a number
        }

        if (value.is_string()) {
            auto str_result = value.to_string();
            if (std::holds_alternative<std::string>(str_result)) {
                const std::string& str = std::get<std::string>(str_result);

                // Handle base parameter if provided
                int base = 10;
                if (args.size() > 1 && args[1].is_number()) {
                    auto base_result = args[1].to_number();
                    if (std::holds_alternative<double>(base_result)) {
                        base = static_cast<int>(std::get<double>(base_result));
                        if (base < 2 || base > 36) {
                            return {runtime::Value()};  // Invalid base
                        }
                    }
                }

                try {
                    if (base == 10) {
                        // Check for hexadecimal prefix in base 10 mode (Lua 5.5 behavior)
                        if (str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
                            // Parse as hexadecimal
                            Int int_val = std::stoll(str, nullptr, 16);
                            return {runtime::Value(static_cast<Number>(int_val))};
                        }

                        // Try integer first
                        if (str.find('.') == std::string::npos &&
                            str.find('e') == std::string::npos &&
                            str.find('E') == std::string::npos) {
                            Int int_val = std::stoll(str);
                            return {runtime::Value(static_cast<Number>(int_val))};
                        } else {
                            // Float
                            double float_val = std::stod(str);
                            return {runtime::Value(float_val)};
                        }
                    } else {
                        // Non-decimal base
                        Int int_val = std::stoll(str, nullptr, base);
                        return {runtime::Value(static_cast<Number>(int_val))};
                    }
                } catch (const std::exception&) {
                    return {runtime::Value()};  // Conversion failed
                }
            }
        }

        return {runtime::Value()};  // nil
    }

    std::vector<runtime::Value> getmetatable(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value()};  // nil
        }

        const auto& value = args[0];

        if (value.is_table()) {
            const auto& table_ptr = value.as_table();
            if (table_ptr) {
                auto metatable = table_ptr->metatable();
                if (metatable) {
                    return {runtime::Value(metatable)};
                }
            }
        } else if (value.is_userdata()) {
            const auto& userdata_ptr = value.as_userdata();
            if (userdata_ptr) {
                auto metatable = userdata_ptr->metatable();
                if (metatable) {
                    return {runtime::Value(metatable)};
                }
            }
        }

        return {runtime::Value()};  // nil
    }

    std::vector<runtime::Value> setmetatable(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.size() < 2) {
            return {runtime::Value()};  // nil
        }

        const auto& table_value = args[0];
        const auto& metatable_value = args[1];

        if (!table_value.is_table()) {
            // In Lua, setmetatable only works on tables
            return {runtime::Value()};  // nil
        }

        const auto& table_ptr = table_value.as_table();
        if (!table_ptr) {
            return {runtime::Value()};  // nil
        }

        if (metatable_value.is_nil()) {
            // Remove metatable
            table_ptr->setMetatable(runtime::GCPtr<runtime::Table>{});
        } else if (metatable_value.is_table()) {
            // Set metatable
            const auto& metatable_ptr = metatable_value.as_table();
            table_ptr->setMetatable(metatable_ptr);
        } else {
            // Invalid metatable type
            return {runtime::Value()};  // nil
        }

        return {table_value};  // Return the table
    }

    std::vector<runtime::Value> rawget(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.size() < 2) {
            return {runtime::Value()};  // nil
        }

        const auto& table_value = args[0];
        const auto& key_value = args[1];

        if (!table_value.is_table()) {
            return {runtime::Value()};  // nil
        }

        auto table_result = table_value.to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {runtime::Value()};  // nil
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
        return {table->get(key_value)};
    }

    std::vector<runtime::Value> rawset(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.size() < 3) {
            return {runtime::Value()};  // nil
        }

        const auto& table_value = args[0];
        const auto& key_value = args[1];
        const auto& value = args[2];

        if (!table_value.is_table()) {
            return {runtime::Value()};  // nil
        }

        auto table_result = table_value.to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {runtime::Value()};  // nil
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
        table->set(key_value, value);
        return {table_value};
    }

    std::vector<runtime::Value> pcall_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            vm->trigger_runtime_error("bad argument #1 to 'pcall' (value expected)");
            return {};
        }

        const auto& func = args[0];
        std::vector<runtime::Value> func_args;
        if (args.size() > 1) {
            func_args.assign(args.begin() + 1, args.end());
        }

        auto result = vm->pcall(func, func_args);
        // vm->pcall now always returns a success Result containing the vector.
        return get_value(result);
    }

    std::vector<runtime::Value> xpcall_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.size() < 2) {
            vm->trigger_runtime_error("bad argument to 'xpcall' (value expected)");
            return {};
        }

        const auto& func = args[0];
        const auto& msgh = args[1];

        std::vector<runtime::Value> func_args;
        if (args.size() > 2) {
            func_args.assign(args.begin() + 2, args.end());
        }

        auto result = vm->xpcall(func, msgh, func_args);
        return get_value(result);
    }

    std::vector<runtime::Value> rawequal(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.size() < 2) {
            return {runtime::Value(false)};
        }

        // Raw equality check without metamethods
        bool equal = (args[0] == args[1]);
        return {runtime::Value(equal)};
    }

    std::vector<runtime::Value> rawlen(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {runtime::Value()};  // nil
        }

        const auto& value = args[0];

        if (value.is_string()) {
            auto str_result = value.to_string();
            if (std::holds_alternative<std::string>(str_result)) {
                const std::string& str = std::get<std::string>(str_result);
                return {runtime::Value(static_cast<Number>(str.length()))};
            }
        } else if (value.is_table()) {
            auto table_result = value.to_table();
            if (std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
                auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
                // rawlen should count consecutive non-nil elements from index 1
                return {runtime::Value(static_cast<Number>(table->rawLength()))};
            }
        }

        return {runtime::Value()};  // nil
    }

    std::vector<runtime::Value> select(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {};
        }

        const auto& index_value = args[0];

        // Handle "#" case - return number of arguments after index
        if (index_value.is_string()) {
            auto str_result = index_value.to_string();
            if (std::holds_alternative<std::string>(str_result)) {
                const std::string& str = std::get<std::string>(str_result);
                if (str == "#") {
                    // Return count of arguments excluding the index argument
                    return {runtime::Value(static_cast<Number>(args.size() - 1))};
                }
            }
        }

        // Handle numeric index
        if (index_value.is_number()) {
            auto num_result = index_value.to_number();
            if (std::holds_alternative<double>(num_result)) {
                int index = static_cast<int>(std::get<double>(num_result));
                int total_args = static_cast<int>(args.size()) - 1;  // Exclude index argument

                // Handle negative indices (count from end)
                if (index < 0) {
                    index = total_args + index + 1;
                }

                // Validate index range (1-based indexing)
                if (index >= 1 && index <= total_args) {
                    std::vector<runtime::Value> result;
                    // Start from the specified index (accounting for 0-based args array)
                    // args[0] is the index, args[1] is the first actual argument
                    for (auto i = static_cast<size_t>(index); i < args.size(); ++i) {
                        result.push_back(args[i]);
                    }
                    return result;
                } else if (index > total_args) {
                    // Index beyond available arguments - return empty
                    return {};
                }
            }
        }

        return {};
    }

    std::vector<runtime::Value> error(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        std::string message = "error";
        if (!args.empty()) {
            // Use tostring to convert the first argument to a string, similar to Lua's error function.
            auto str_result = runtime::Value::tostring_with_metamethod(args[0]);
             if (std::holds_alternative<std::string>(str_result)) {
                message = std::get<std::string>(str_result);
            } else {
                message = args[0].debug_string();
            }
        }

        // Use the VM's error handling mechanism. This will set the VM state
        // to Error and allow pcall/xpcall to catch it.
        vm->trigger_runtime_error(message);

        // The error function should not return.
        // The VM will stop execution after trigger_runtime_error.
        return {};
    }

    std::vector<runtime::Value> assert_(runtime::IVMContext* vm, const std::vector<runtime::Value>& args) {
        if (args.empty() || args[0].is_falsy()) {
            std::string message = "assertion failed!";
            if (args.size() > 1) {
                // Use tostring to convert the second argument to a string.
                auto str_result = runtime::Value::tostring_with_metamethod(args[1]);
                if (std::holds_alternative<std::string>(str_result)) {
                    message = std::get<std::string>(str_result);
                } else {
                    message = args[1].debug_string();
                }
            }
            vm->trigger_runtime_error(message);
            return {}; // Should not be reached if error handling works correctly
        }

        // Return all arguments on success
        return args;
    }

    void register_functions(runtime::IVMContext* vm, const runtime::GCPtr<runtime::Table>& globals) {
        // Register the basic library functions
        globals->set(runtime::Value("print"), runtime::value_factory::function(print, vm));
        globals->set(runtime::Value("type"), runtime::value_factory::function(type, vm));
        globals->set(runtime::Value("ipairs"), runtime::value_factory::function(ipairs, vm));
        globals->set(runtime::Value("pairs"), runtime::value_factory::function(pairs, vm));
        globals->set(runtime::Value("next"), runtime::value_factory::function(next, vm));
        globals->set(runtime::Value("tostring"), runtime::value_factory::function(tostring, vm));
        globals->set(runtime::Value("tonumber"), runtime::value_factory::function(tonumber, vm));
        globals->set(runtime::Value("getmetatable"),
                     runtime::value_factory::function(getmetatable, vm));
        globals->set(runtime::Value("setmetatable"),
                     runtime::value_factory::function(setmetatable, vm));
        globals->set(runtime::Value("rawget"), runtime::value_factory::function(rawget, vm));
        globals->set(runtime::Value("rawset"), runtime::value_factory::function(rawset, vm));
        globals->set(runtime::Value("rawequal"), runtime::value_factory::function(rawequal, vm));
        globals->set(runtime::Value("rawlen"), runtime::value_factory::function(rawlen, vm));
        globals->set(runtime::Value("select"), runtime::value_factory::function(select, vm));
        globals->set(runtime::Value("error"), runtime::value_factory::function(error, vm));
        globals->set(runtime::Value("assert"), runtime::value_factory::function(assert_, vm));
        globals->set(runtime::Value("pcall"), runtime::value_factory::function(pcall_, vm));
        globals->set(runtime::Value("xpcall"), runtime::value_factory::function(xpcall_, vm));
    }

}  // namespace rangelua::stdlib::basic
