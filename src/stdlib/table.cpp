/**
 * @file table.cpp
 * @brief Implementation of Lua table library functions for RangeLua
 * @version 0.1.0
 */

#include <rangelua/stdlib/table.hpp>

#include <algorithm>
#include <sstream>

#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>

namespace rangelua::stdlib::table {

    std::vector<runtime::Value> concat(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_table()) {
            return {runtime::Value("")};
        }

        auto table_result = args[0].to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {runtime::Value("")};
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
        
        std::string separator;
        if (args.size() > 1 && args[1].is_string()) {
            auto sep_result = args[1].to_string();
            if (std::holds_alternative<std::string>(sep_result)) {
                separator = std::get<std::string>(sep_result);
            }
        }

        size_t start = 1;
        size_t end = table->arraySize();

        if (args.size() > 2 && args[2].is_number()) {
            auto start_result = args[2].to_number();
            if (std::holds_alternative<double>(start_result)) {
                start = static_cast<size_t>(std::max(1.0, std::get<double>(start_result)));
            }
        }

        if (args.size() > 3 && args[3].is_number()) {
            auto end_result = args[3].to_number();
            if (std::holds_alternative<double>(end_result)) {
                end = static_cast<size_t>(std::max(1.0, std::get<double>(end_result)));
            }
        }

        std::ostringstream result;
        for (size_t i = start; i <= end && i <= table->arraySize(); ++i) {
            if (i > start && !separator.empty()) {
                result << separator;
            }
            
            auto value = table->getArray(i);
            if (value.is_string()) {
                auto str_result = value.to_string();
                if (std::holds_alternative<std::string>(str_result)) {
                    result << std::get<std::string>(str_result);
                }
            } else if (value.is_number()) {
                auto num_result = value.to_number();
                if (std::holds_alternative<double>(num_result)) {
                    result << std::get<double>(num_result);
                }
            }
        }

        return {runtime::Value(result.str())};
    }

    std::vector<runtime::Value> insert(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_table()) {
            return {};
        }

        auto table_result = args[0].to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {};
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);

        if (args.size() == 2) {
            // Insert at end
            size_t pos = table->arraySize() + 1;
            table->setArray(pos, args[1]);
        } else if (args.size() >= 3) {
            // Insert at specified position
            if (args[1].is_number()) {
                auto pos_result = args[1].to_number();
                if (std::holds_alternative<double>(pos_result)) {
                    size_t pos = static_cast<size_t>(std::get<double>(pos_result));
                    
                    // Shift elements to the right
                    size_t array_size = table->arraySize();
                    for (size_t i = array_size; i >= pos; --i) {
                        auto value = table->getArray(i);
                        table->setArray(i + 1, value);
                        if (i == pos) break;  // Prevent underflow
                    }
                    
                    // Insert new element
                    table->setArray(pos, args[2]);
                }
            }
        }

        return {};
    }

    std::vector<runtime::Value> move(const std::vector<runtime::Value>& args) {
        if (args.size() < 4) {
            return {};
        }

        if (!args[0].is_table() || !args[1].is_number() || !args[2].is_number() || !args[3].is_number()) {
            return {};
        }

        auto source_result = args[0].to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(source_result)) {
            return {};
        }

        auto source = std::get<runtime::GCPtr<runtime::Table>>(source_result);
        
        // Destination table (default to source if not provided)
        auto dest = source;
        if (args.size() > 4 && args[4].is_table()) {
            auto dest_result = args[4].to_table();
            if (std::holds_alternative<runtime::GCPtr<runtime::Table>>(dest_result)) {
                dest = std::get<runtime::GCPtr<runtime::Table>>(dest_result);
            }
        }

        auto start_result = args[1].to_number();
        auto end_result = args[2].to_number();
        auto dest_start_result = args[3].to_number();

        if (std::holds_alternative<double>(start_result) && 
            std::holds_alternative<double>(end_result) && 
            std::holds_alternative<double>(dest_start_result)) {
            
            size_t start = static_cast<size_t>(std::get<double>(start_result));
            size_t end = static_cast<size_t>(std::get<double>(end_result));
            size_t dest_start = static_cast<size_t>(std::get<double>(dest_start_result));

            for (size_t i = start; i <= end; ++i) {
                auto value = source->getArray(i);
                dest->setArray(dest_start + (i - start), value);
            }
        }

        return {runtime::Value(dest)};
    }

    std::vector<runtime::Value> pack(const std::vector<runtime::Value>& args) {
        auto table = runtime::value_factory::table();
        auto table_result = table.to_table();
        
        if (std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            auto table_ptr = std::get<runtime::GCPtr<runtime::Table>>(table_result);
            
            // Pack arguments into array part
            for (size_t i = 0; i < args.size(); ++i) {
                table_ptr->setArray(i + 1, args[i]);
            }
            
            // Set "n" field to number of arguments
            table_ptr->set(runtime::Value("n"), runtime::Value(static_cast<double>(args.size())));
        }

        return {table};
    }

    std::vector<runtime::Value> remove(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_table()) {
            return {};
        }

        auto table_result = args[0].to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {};
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
        
        size_t pos = table->arraySize();  // Default to last element
        
        if (args.size() > 1 && args[1].is_number()) {
            auto pos_result = args[1].to_number();
            if (std::holds_alternative<double>(pos_result)) {
                pos = static_cast<size_t>(std::get<double>(pos_result));
            }
        }

        if (pos == 0 || pos > table->arraySize()) {
            return {runtime::Value()};  // nil
        }

        // Get the element to remove
        auto removed = table->getArray(pos);
        
        // Shift elements to the left
        for (size_t i = pos; i < table->arraySize(); ++i) {
            auto value = table->getArray(i + 1);
            table->setArray(i, value);
        }
        
        // Remove the last element
        table->setArray(table->arraySize(), runtime::Value());

        return {removed};
    }

    std::vector<runtime::Value> sort(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_table()) {
            return {};
        }

        auto table_result = args[0].to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {};
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
        
        // Simple bubble sort implementation for now
        // TODO: Implement proper sorting with comparison function support
        size_t n = table->arraySize();
        
        for (size_t i = 1; i <= n; ++i) {
            for (size_t j = 1; j <= n - i; ++j) {
                auto a = table->getArray(j);
                auto b = table->getArray(j + 1);
                
                // Simple numeric comparison
                bool should_swap = false;
                if (a.is_number() && b.is_number()) {
                    auto a_result = a.to_number();
                    auto b_result = b.to_number();
                    if (std::holds_alternative<double>(a_result) && std::holds_alternative<double>(b_result)) {
                        should_swap = std::get<double>(a_result) > std::get<double>(b_result);
                    }
                }
                
                if (should_swap) {
                    table->setArray(j, b);
                    table->setArray(j + 1, a);
                }
            }
        }

        return {};
    }

    std::vector<runtime::Value> unpack(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_table()) {
            return {};
        }

        auto table_result = args[0].to_table();
        if (!std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_result)) {
            return {};
        }

        auto table = std::get<runtime::GCPtr<runtime::Table>>(table_result);
        
        size_t start = 1;
        size_t end = table->arraySize();

        if (args.size() > 1 && args[1].is_number()) {
            auto start_result = args[1].to_number();
            if (std::holds_alternative<double>(start_result)) {
                start = static_cast<size_t>(std::max(1.0, std::get<double>(start_result)));
            }
        }

        if (args.size() > 2 && args[2].is_number()) {
            auto end_result = args[2].to_number();
            if (std::holds_alternative<double>(end_result)) {
                end = static_cast<size_t>(std::max(1.0, std::get<double>(end_result)));
            }
        }

        std::vector<runtime::Value> result;
        for (size_t i = start; i <= end && i <= table->arraySize(); ++i) {
            result.push_back(table->getArray(i));
        }

        return result;
    }

    void register_functions(const runtime::GCPtr<runtime::Table>& globals) {
        // Create table table
        auto table_table = runtime::value_factory::table();
        auto table_table_ptr = table_table.to_table();
        
        if (std::holds_alternative<runtime::GCPtr<runtime::Table>>(table_table_ptr)) {
            auto table = std::get<runtime::GCPtr<runtime::Table>>(table_table_ptr);
            
            // Register table library functions
            table->set(runtime::Value("concat"), runtime::value_factory::function(concat));
            table->set(runtime::Value("insert"), runtime::value_factory::function(insert));
            table->set(runtime::Value("move"), runtime::value_factory::function(move));
            table->set(runtime::Value("pack"), runtime::value_factory::function(pack));
            table->set(runtime::Value("remove"), runtime::value_factory::function(remove));
            table->set(runtime::Value("sort"), runtime::value_factory::function(sort));
            table->set(runtime::Value("unpack"), runtime::value_factory::function(unpack));
            
            // Register the table table in globals
            globals->set(runtime::Value("table"), table_table);
        }
    }

}  // namespace rangelua::stdlib::table
