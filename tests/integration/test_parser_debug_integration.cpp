/**
 * @file test_parser_debug_integration.cpp
 * @brief Test parser debug and error integration
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/frontend/parser.hpp>
#include <rangelua/utils/logger.hpp>
#include <rangelua/utils/debug.hpp>

using namespace rangelua;
using namespace rangelua::frontend;

TEST_CASE("Parser debug integration", "[frontend][parser][debug]") {
    // Initialize logging for tests
    utils::Logger::initialize("test_parser_debug", utils::Logger::LogLevel::trace);

    SECTION("Debug mode enabled during parsing") {
        String source = R"(
            local x = 42
            local y = x + 10
            function test()
                return x + y
            end
        )";

        frontend::Parser parser(source, "test_debug.lua");
        auto result = parser.parse();

        REQUIRE(is_success(result));
        REQUIRE_FALSE(parser.has_errors());

        // In debug mode, we should have detailed logging
        // This test mainly verifies that debug integration doesn't break parsing
    }

    SECTION("Error handling with debug context") {
        String source = R"(
            local x = 42
            local y = -- missing value
            function test(
                -- missing closing parenthesis and body
        )";

        frontend::Parser parser(source, "test_errors.lua");
        auto result = parser.parse();

        // Should still return a result (with error recovery)
        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());

        const auto& errors = parser.errors();
        REQUIRE_FALSE(errors.empty());

        // Verify error messages contain enhanced context
        bool found_enhanced_error = false;
        for (const auto& error : errors) {
            String error_msg = error.what();
            if (error_msg.find("expected:") != String::npos ||
                error_msg.find("Suggestion:") != String::npos) {
                found_enhanced_error = true;
                break;
            }
        }
        REQUIRE(found_enhanced_error);
    }

    SECTION("Expression depth tracking") {
        // Create a deeply nested expression
        String source = "local x = ((((((1 + 2) + 3) + 4) + 5) + 6) + 7)";

        frontend::Parser parser(source, "test_depth.lua");
        auto result = parser.parse();

        REQUIRE(is_success(result));
        REQUIRE_FALSE(parser.has_errors());
    }

    SECTION("Expression depth limit exceeded") {
        // Create an expression that exceeds the default depth limit
        String source = "local x = ";
        for (int i = 0; i < 250; ++i) {
            source += "((";
        }
        source += "1";
        for (int i = 0; i < 250; ++i) {
            source += "))";
        }

        frontend::Parser parser(source, "test_depth_limit.lua");
        auto result = parser.parse();

        // Should still return a result due to error recovery
        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());

        // Should have an error about depth limit
        const auto& errors = parser.errors();
        bool found_depth_error = false;
        for (const auto& error : errors) {
            String error_msg = error.what();
            if (error_msg.find("depth limit exceeded") != String::npos) {
                found_depth_error = true;
                break;
            }
        }
        REQUIRE(found_depth_error);
    }

    SECTION("Error recovery with synchronization") {
        String source = R"(
            local x = 42
            invalid syntax here !!!
            local y = 10
            function test()
                return x + y
            end
        )";

        frontend::Parser parser(source, "test_recovery.lua");
        auto result = parser.parse();

        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());

        // Should have recovered and parsed the valid parts
        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE_FALSE(program->statements().empty());
    }

    SECTION("Enhanced error messages") {
        String source = R"(
            function test(
                -- missing closing parenthesis
            local x = 42
        )";

        frontend::Parser parser(source, "test_enhanced_errors.lua");
        auto result = parser.parse();

        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());

        const auto& errors = parser.errors();
        REQUIRE_FALSE(errors.empty());

        // Check that errors have enhanced information
        for (const auto& error : errors) {
            String error_msg = error.what();
            // Enhanced errors should contain additional context
            REQUIRE_FALSE(error_msg.empty());
        }
    }
}

TEST_CASE("Parser error suggestions", "[frontend][parser][error]") {
    SECTION("Missing semicolon suggestion") {
        String source = R"(
            local x = 42
            function test(
                -- missing closing parenthesis
            local y = 10
        )";

        frontend::Parser parser(source, "test_suggestions.lua");
        auto result = parser.parse();

        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());

        // Should have suggestions in error messages
        const auto& errors = parser.errors();
        bool found_suggestion = false;
        for (const auto& error : errors) {
            String error_msg = error.what();
            if (error_msg.find("Suggestion:") != String::npos) {
                found_suggestion = true;
                break;
            }
        }
        REQUIRE(found_suggestion);
    }
}

TEST_CASE("Parser debug state tracking", "[frontend][parser][debug]") {
    SECTION("Token consumption tracking") {
        String source = R"(
            local x = 42
            local y = x + 10
            return x + y
        )";

        frontend::Parser parser(source, "test_tracking.lua");
        auto result = parser.parse();

        REQUIRE(is_success(result));
        REQUIRE_FALSE(parser.has_errors());

        // Debug tracking should work without affecting parsing
        // This mainly tests that debug code doesn't break functionality
    }
}
