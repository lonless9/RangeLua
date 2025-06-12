#include <rangelua/rangelua.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
    // Path to the official Lua executable from the git submodule.
    // First try the submodule version, then fall back to system Lua.
    constexpr const char* SUBMODULE_LUA_PATH = "third_party/lua/lua";

    // Helper function to find all Lua test files recursively in a directory.
    std::vector<std::string> find_lua_test_files(const std::string& directory) {
        std::vector<std::string> files;
        if (!fs::exists(directory)) {
            return files;
        }
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                files.push_back(entry.path().string());
            }
        }
        std::ranges::sort(files);
        return files;
    }

    // Helper function to parse expected output from a Lua file's comments.
    // Expected format:
    // -- Expected output:
    // -- line 1
    // -- line 2
    std::string parse_expected_output(const std::string& file_path) {
        std::ifstream file(file_path);
        std::string line;
        std::stringstream expected_ss;
        bool in_expected_block = false;

        while (std::getline(file, line)) {
            std::smatch match;
            if (!in_expected_block &&
                std::regex_search(line, match, std::regex(R"(--\s*Expected output:.*)"))) {
                in_expected_block = true;
                continue;
            }

            if (in_expected_block) {
                if (line.starts_with("--")) {
                    // Remove the "--" and optional space
                    expected_ss << std::regex_replace(line, std::regex(R"(--\s?)"), "") << '\n';
                } else {
                    // End of the comment block
                    break;
                }
            }
        }

        std::string result = expected_ss.str();
        // Trim single trailing newline for consistency
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return result;
    }

    // Helper function to execute a command and capture its stdout.
    // Note: Using popen() is intentional for testing purposes to execute external Lua interpreter
    std::string execute_command(const std::string& command) {
        std::array<char, 256> buffer{};  // Initialize to zero
        std::string result;
        // NOLINTNEXTLINE(cert-env33-c) - popen is intentional for testing external Lua
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed for command: " + command);
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        // Trim single trailing newline for consistency
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return result;
    }

    // Helper function to normalize output for comparison
    // This handles differences like memory addresses that will naturally differ
    std::string normalize_output_for_comparison(const std::string& output) {
        std::string normalized = output;

        // Replace memory addresses (hexadecimal patterns like 0x7f1234567890)
        // with a placeholder to avoid comparison failures due to different memory layouts
        std::regex memory_addr_pattern(R"(0x[0-9a-fA-F]+)");
        normalized = std::regex_replace(normalized, memory_addr_pattern, "0xMEMORY_ADDR");

        // Replace table/function addresses in Lua output (e.g., "table: 0x...")
        std::regex table_addr_pattern(R"((table|function|thread|userdata):\s*0x[0-9a-fA-F]+)");
        normalized = std::regex_replace(normalized, table_addr_pattern, "$1");

        // Normalize line numbers in error messages to "?:"
        std::regex line_number_pattern(R"((tests/scripts/[^:]+):\d+:)");
        normalized = std::regex_replace(normalized, line_number_pattern, "$1:?:");

        return normalized;
    }

}  // anonymous namespace

// Main test case for running Lua scripts
// Note: Catch2 macros generate unavoidable static analysis warnings
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wunused-template"
TEST_CASE("Lua Script Integration Tests", "[integration]") {
    // Discover all .lua files in the tests/scripts directory
    static auto test_files = find_lua_test_files("tests/scripts");
    REQUIRE_FALSE(test_files.empty());

    // Use Catch2's generator to create a test for each file
    auto file_path = GENERATE(from_range(test_files));

    DYNAMIC_SECTION("Test Script: " << file_path) {
        // Parse the expected output from the file
        std::string expected_output = parse_expected_output(file_path);

        // --- Execute with RangeLua ---
        std::string rangelua_output;
        {
            // Redirect stdout to capture output
            std::stringstream buffer;
            auto* old_cout = std::cout.rdbuf(buffer.rdbuf());

            // Execute the script
            rangelua::api::State state;
            auto result = state.execute_file(file_path);

            // Restore stdout
            std::cout.rdbuf(old_cout);

            // Check for execution errors
            if (rangelua::is_error(result)) {
                FAIL("RangeLua execution failed for "
                     << file_path
                     << " with error code: " << static_cast<int>(rangelua::get_error(result)));
            }

            rangelua_output = buffer.str();
            // Trim single trailing newline to normalize
            if (!rangelua_output.empty() && rangelua_output.back() == '\n') {
                rangelua_output.pop_back();
            }
        }

        // --- Execute with Official Lua (if available) ---
        std::string official_lua_output;
        bool official_lua_ran = false;
        try {
            std::string command = std::string("./") + SUBMODULE_LUA_PATH + " " + file_path;
            official_lua_output = execute_command(command);
            official_lua_ran = true;
        } catch (const std::exception& e) {
            WARN("Could not execute official Lua for comparison: " << e.what());
        }

        // --- Compare results ---

        // Check RangeLua against the official Lua output if it ran
        if (official_lua_ran) {
            INFO("Comparing RangeLua output against official Lua output.");
            INFO("File: " << file_path);
            INFO("Official Lua output:\n" << official_lua_output);
            INFO("RangeLua output:\n" << rangelua_output);

            // Normalize both outputs to handle memory address differences
            std::string normalized_official = normalize_output_for_comparison(official_lua_output);
            std::string normalized_rangelua = normalize_output_for_comparison(rangelua_output);

            INFO("Normalized Official Lua output:\n" << normalized_official);
            INFO("Normalized RangeLua output:\n" << normalized_rangelua);
            CHECK(normalized_rangelua == normalized_official);
        } else {
            // Otherwise, fall back to comparing against the embedded expected output
            INFO("Comparing RangeLua output against embedded expectation (Official Lua not run).");
            INFO("File: " << file_path);
            INFO("Expected output:\n" << expected_output);
            INFO("RangeLua output:\n" << rangelua_output);

            // Also normalize the expected output comparison
            std::string normalized_expected = normalize_output_for_comparison(expected_output);
            std::string normalized_rangelua = normalize_output_for_comparison(rangelua_output);

            INFO("Normalized Expected output:\n" << normalized_expected);
            INFO("Normalized RangeLua output:\n" << normalized_rangelua);
            CHECK(normalized_rangelua == normalized_expected);
        }
    }
}

// Test case for validating RangeLua behavior against official Lua
// Note: This is primarily for validation, not for passing all official tests
TEST_CASE("Lua Official Implementation Validation", "[validation][official]") {
    // Only test very basic functionality that RangeLua should already support
    // This is for validation purposes, not comprehensive testing
    static const std::vector<std::string> validation_scripts = {
        // These are simple validation scripts, not the full official test suite
        // Add only when RangeLua can handle them
    };

    if (validation_scripts.empty()) {
        SKIP("No validation scripts configured yet. Official Lua submodule is available for future "
             "validation.");
    }

    // This test case is prepared for future use when RangeLua is more mature
    // For now, it serves as a placeholder and documentation of the validation approach
}
#pragma clang diagnostic pop
