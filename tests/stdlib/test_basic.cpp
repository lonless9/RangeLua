/**
 * @file test_basic.cpp
 * @brief Tests for basic library functions
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>

#include <rangelua/stdlib/basic.hpp>
#include <rangelua/runtime/value.hpp>

using namespace rangelua;

TEST_CASE("Basic library print function", "[stdlib][basic][print]") {
    SECTION("Print with no arguments") {
        // Capture stdout
        std::ostringstream captured_output;
        std::streambuf* orig = std::cout.rdbuf();
        std::cout.rdbuf(captured_output.rdbuf());

        // Call print with no arguments
        auto result = stdlib::basic::print({});

        // Restore stdout
        std::cout.rdbuf(orig);

        // Check result
        REQUIRE(result.empty());  // print returns no values
        REQUIRE(captured_output.str() == "\n");  // Just a newline
    }

    SECTION("Print with single string argument") {
        // Capture stdout
        std::ostringstream captured_output;
        std::streambuf* orig = std::cout.rdbuf();
        std::cout.rdbuf(captured_output.rdbuf());

        // Call print with string argument
        std::vector<runtime::Value> args = {runtime::Value("Hello, World!")};
        auto result = stdlib::basic::print(args);

        // Restore stdout
        std::cout.rdbuf(orig);

        // Check result
        REQUIRE(result.empty());  // print returns no values
        REQUIRE(captured_output.str() == "Hello, World!\n");
    }

    SECTION("Print with multiple arguments") {
        // Capture stdout
        std::ostringstream captured_output;
        std::streambuf* orig = std::cout.rdbuf();
        std::cout.rdbuf(captured_output.rdbuf());

        // Call print with multiple arguments
        std::vector<runtime::Value> args = {
            runtime::Value("Hello"),
            runtime::Value(42.0),
            runtime::Value(true),
            runtime::Value()  // nil
        };
        auto result = stdlib::basic::print(args);

        // Restore stdout
        std::cout.rdbuf(orig);

        // Check result
        REQUIRE(result.empty());  // print returns no values
        REQUIRE(captured_output.str() == "Hello\t42\ttrue\tnil\n");
    }

    SECTION("Print with number formatting") {
        // Capture stdout
        std::ostringstream captured_output;
        std::streambuf* orig = std::cout.rdbuf();
        std::cout.rdbuf(captured_output.rdbuf());

        // Call print with different number types
        std::vector<runtime::Value> args = {
            runtime::Value(42.0),    // Integer
            runtime::Value(3.14),    // Float
            runtime::Value(0.0)      // Zero
        };
        auto result = stdlib::basic::print(args);

        // Restore stdout
        std::cout.rdbuf(orig);

        // Check result
        REQUIRE(result.empty());  // print returns no values
        std::string output = captured_output.str();
        REQUIRE(output.find("42") != std::string::npos);
        REQUIRE(output.find("3.14") != std::string::npos);
        REQUIRE(output.find("0") != std::string::npos);
    }
}

TEST_CASE("Basic library type function", "[stdlib][basic][type]") {
    SECTION("Type of nil") {
        std::vector<runtime::Value> args = {runtime::Value()};
        auto result = stdlib::basic::type(args);

        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_string());
        auto str_result = result[0].to_string();
        REQUIRE(std::holds_alternative<std::string>(str_result));
        // The exact type name depends on the Value implementation
    }

    SECTION("Type of boolean") {
        std::vector<runtime::Value> args = {runtime::Value(true)};
        auto result = stdlib::basic::type(args);

        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_string());
        auto str_result = result[0].to_string();
        REQUIRE(std::holds_alternative<std::string>(str_result));
    }

    SECTION("Type of number") {
        std::vector<runtime::Value> args = {runtime::Value(42.0)};
        auto result = stdlib::basic::type(args);

        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_string());
        auto str_result = result[0].to_string();
        REQUIRE(std::holds_alternative<std::string>(str_result));
    }

    SECTION("Type of string") {
        std::vector<runtime::Value> args = {runtime::Value("hello")};
        auto result = stdlib::basic::type(args);

        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_string());
        auto str_result = result[0].to_string();
        REQUIRE(std::holds_alternative<std::string>(str_result));
    }

    SECTION("Type with no arguments") {
        std::vector<runtime::Value> args = {};
        auto result = stdlib::basic::type(args);

        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_string());
        auto str_result = result[0].to_string();
        REQUIRE(std::holds_alternative<std::string>(str_result));
        REQUIRE(std::get<std::string>(str_result) == "nil");
    }
}
