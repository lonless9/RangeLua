/**
 * @file string.cpp
 * @brief Implementation of Lua string library functions for RangeLua
 * @version 0.1.0
 */

#include <rangelua/stdlib/string.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>
#include <sstream>

#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>

namespace rangelua::stdlib::string {

    std::vector<runtime::Value> byte(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {};
        }

        auto str_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(str_result)) {
            return {};
        }

        const std::string& str = std::get<std::string>(str_result);
        if (str.empty()) {
            return {};
        }

        // Default to first character
        size_t start = 1;
        size_t end = 1;

        // Parse start position
        if (args.size() > 1 && args[1].is_number()) {
            auto num_result = args[1].to_number();
            if (std::holds_alternative<double>(num_result)) {
                int pos = static_cast<int>(std::get<double>(num_result));
                if (pos < 0) {
                    pos = static_cast<int>(str.length()) + pos + 1;
                }
                start = std::max(1, pos);
            }
        }

        // Parse end position
        if (args.size() > 2 && args[2].is_number()) {
            auto num_result = args[2].to_number();
            if (std::holds_alternative<double>(num_result)) {
                int pos = static_cast<int>(std::get<double>(num_result));
                if (pos < 0) {
                    pos = static_cast<int>(str.length()) + pos + 1;
                }
                end = std::max(1, pos);
            }
        } else {
            end = start;
        }

        std::vector<runtime::Value> result;
        for (size_t i = start; i <= end && i <= str.length(); ++i) {
            unsigned char c = static_cast<unsigned char>(str[i - 1]);
            result.push_back(runtime::Value(static_cast<double>(c)));
        }

        return result;
    }

    std::vector<runtime::Value> char_(const std::vector<runtime::Value>& args) {
        std::string result;

        for (const auto& arg : args) {
            if (arg.is_number()) {
                auto num_result = arg.to_number();
                if (std::holds_alternative<double>(num_result)) {
                    int code = static_cast<int>(std::get<double>(num_result));
                    if (code >= 0 && code <= 255) {
                        result += static_cast<char>(code);
                    }
                }
            }
        }

        return {runtime::Value(result)};
    }

    std::vector<runtime::Value> find(const std::vector<runtime::Value>& args) {
        if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) {
            return {};
        }

        auto str_result = args[0].to_string();
        auto pattern_result = args[1].to_string();

        if (!std::holds_alternative<std::string>(str_result) ||
            !std::holds_alternative<std::string>(pattern_result)) {
            return {};
        }

        const std::string& str = std::get<std::string>(str_result);
        const std::string& pattern = std::get<std::string>(pattern_result);

        size_t start_pos = 1;
        if (args.size() > 2 && args[2].is_number()) {
            auto num_result = args[2].to_number();
            if (std::holds_alternative<double>(num_result)) {
                int pos = static_cast<int>(std::get<double>(num_result));
                if (pos < 0) {
                    pos = static_cast<int>(str.length()) + pos + 1;
                }
                start_pos = std::max(1, pos);
            }
        }

        if (start_pos > str.length()) {
            return {};
        }

        // Simple string search (not full pattern matching for now)
        size_t found = str.find(pattern, start_pos - 1);
        if (found != std::string::npos) {
            return {
                runtime::Value(static_cast<double>(found + 1)),
                runtime::Value(static_cast<double>(found + pattern.length()))
            };
        }

        return {};
    }

    std::vector<runtime::Value> format(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {runtime::Value("")};
        }

        auto format_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(format_result)) {
            return {runtime::Value("")};
        }

        const std::string& format_str = std::get<std::string>(format_result);

        // Simple implementation - just return format string for now
        // TODO: Implement full printf-style formatting
        return {runtime::Value(format_str)};
    }

    std::vector<runtime::Value> gsub(const std::vector<runtime::Value>& args) {
        if (args.size() < 3 || !args[0].is_string() || !args[1].is_string()) {
            return {};
        }

        auto str_result = args[0].to_string();
        auto pattern_result = args[1].to_string();

        if (!std::holds_alternative<std::string>(str_result) ||
            !std::holds_alternative<std::string>(pattern_result)) {
            return {};
        }

        std::string str = std::get<std::string>(str_result);
        const std::string& pattern = std::get<std::string>(pattern_result);

        std::string replacement;
        if (args[2].is_string()) {
            auto repl_result = args[2].to_string();
            if (std::holds_alternative<std::string>(repl_result)) {
                replacement = std::get<std::string>(repl_result);
            }
        }

        // Simple string replacement (not full pattern matching)
        size_t count = 0;
        size_t pos = 0;
        while ((pos = str.find(pattern, pos)) != std::string::npos) {
            str.replace(pos, pattern.length(), replacement);
            pos += replacement.length();
            count++;
        }

        return {runtime::Value(str), runtime::Value(static_cast<double>(count))};
    }

    std::vector<runtime::Value> len(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {runtime::Value(0.0)};
        }

        auto str_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(str_result)) {
            return {runtime::Value(0.0)};
        }

        const std::string& str = std::get<std::string>(str_result);
        return {runtime::Value(static_cast<double>(str.length()))};
    }

    std::vector<runtime::Value> lower(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {runtime::Value("")};
        }

        auto str_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(str_result)) {
            return {runtime::Value("")};
        }

        std::string str = std::get<std::string>(str_result);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return {runtime::Value(str)};
    }

    std::vector<runtime::Value> match(const std::vector<runtime::Value>& args) {
        // For now, just return the same as find but only the matched string
        auto find_result = find(args);
        if (find_result.size() >= 2) {
            // Extract the matched substring
            auto str_result = args[0].to_string();
            auto pattern_result = args[1].to_string();

            if (std::holds_alternative<std::string>(str_result) &&
                std::holds_alternative<std::string>(pattern_result)) {

                [[maybe_unused]] const std::string& str = std::get<std::string>(str_result);
                const std::string& pattern = std::get<std::string>(pattern_result);

                return {runtime::Value(pattern)};  // Simple implementation
            }
        }
        return {};
    }

    std::vector<runtime::Value> rep(const std::vector<runtime::Value>& args) {
        if (args.size() < 2 || !args[0].is_string() || !args[1].is_number()) {
            return {runtime::Value("")};
        }

        auto str_result = args[0].to_string();
        auto count_result = args[1].to_number();

        if (!std::holds_alternative<std::string>(str_result) ||
            !std::holds_alternative<double>(count_result)) {
            return {runtime::Value("")};
        }

        const std::string& str = std::get<std::string>(str_result);
        int count = static_cast<int>(std::get<double>(count_result));

        if (count <= 0) {
            return {runtime::Value("")};
        }

        std::string separator;
        if (args.size() > 2 && args[2].is_string()) {
            auto sep_result = args[2].to_string();
            if (std::holds_alternative<std::string>(sep_result)) {
                separator = std::get<std::string>(sep_result);
            }
        }

        std::string result;
        for (int i = 0; i < count; ++i) {
            if (i > 0 && !separator.empty()) {
                result += separator;
            }
            result += str;
        }

        return {runtime::Value(result)};
    }

    std::vector<runtime::Value> reverse(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {runtime::Value("")};
        }

        auto str_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(str_result)) {
            return {runtime::Value("")};
        }

        std::string str = std::get<std::string>(str_result);
        std::reverse(str.begin(), str.end());
        return {runtime::Value(str)};
    }

    std::vector<runtime::Value> sub(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {runtime::Value("")};
        }

        auto str_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(str_result)) {
            return {runtime::Value("")};
        }

        const std::string& str = std::get<std::string>(str_result);
        if (str.empty()) {
            return {runtime::Value("")};
        }

        int start = 1;
        int end = static_cast<int>(str.length());

        // Parse start position
        if (args.size() > 1 && args[1].is_number()) {
            auto num_result = args[1].to_number();
            if (std::holds_alternative<double>(num_result)) {
                start = static_cast<int>(std::get<double>(num_result));
                if (start < 0) {
                    start = static_cast<int>(str.length()) + start + 1;
                }
            }
        }

        // Parse end position
        if (args.size() > 2 && args[2].is_number()) {
            auto num_result = args[2].to_number();
            if (std::holds_alternative<double>(num_result)) {
                end = static_cast<int>(std::get<double>(num_result));
                if (end < 0) {
                    end = static_cast<int>(str.length()) + end + 1;
                }
            }
        }

        // Clamp to valid range
        start = std::max(1, start);
        end = std::min(static_cast<int>(str.length()), end);

        if (start > end) {
            return {runtime::Value("")};
        }

        std::string result = str.substr(start - 1, end - start + 1);
        return {runtime::Value(result)};
    }

    std::vector<runtime::Value> upper(const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_string()) {
            return {runtime::Value("")};
        }

        auto str_result = args[0].to_string();
        if (!std::holds_alternative<std::string>(str_result)) {
            return {runtime::Value("")};
        }

        std::string str = std::get<std::string>(str_result);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return {runtime::Value(str)};
    }

    void register_functions(const runtime::GCPtr<runtime::Table>& globals) {
        // Create string table
        auto string_table = runtime::value_factory::table();
        auto string_table_ptr = string_table.to_table();

        if (std::holds_alternative<runtime::GCPtr<runtime::Table>>(string_table_ptr)) {
            auto table = std::get<runtime::GCPtr<runtime::Table>>(string_table_ptr);

            // Register string library functions
            table->set(runtime::Value("byte"), runtime::value_factory::function(byte));
            table->set(runtime::Value("char"), runtime::value_factory::function(char_));
            table->set(runtime::Value("find"), runtime::value_factory::function(find));
            table->set(runtime::Value("format"), runtime::value_factory::function(format));
            table->set(runtime::Value("gsub"), runtime::value_factory::function(gsub));
            table->set(runtime::Value("len"), runtime::value_factory::function(len));
            table->set(runtime::Value("lower"), runtime::value_factory::function(lower));
            table->set(runtime::Value("match"), runtime::value_factory::function(match));
            table->set(runtime::Value("rep"), runtime::value_factory::function(rep));
            table->set(runtime::Value("reverse"), runtime::value_factory::function(reverse));
            table->set(runtime::Value("sub"), runtime::value_factory::function(sub));
            table->set(runtime::Value("upper"), runtime::value_factory::function(upper));

            // Register the string table in globals
            globals->set(runtime::Value("string"), string_table);
        }
    }

}  // namespace rangelua::stdlib::string
