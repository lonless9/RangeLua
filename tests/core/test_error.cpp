/**
 * @file test_error.cpp
 * @brief Test cases for error handling system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <rangelua/core/error.hpp>
#include <rangelua/utils/logger.hpp>

using namespace rangelua;

TEST_CASE("ErrorCode enum values", "[core][error]") {
    SECTION("ErrorCode has expected values") {
        REQUIRE(static_cast<int>(ErrorCode::SUCCESS) == 0);
        REQUIRE(static_cast<int>(ErrorCode::SYNTAX_ERROR) == 1);
        REQUIRE(static_cast<int>(ErrorCode::RUNTIME_ERROR) == 2);
        REQUIRE(static_cast<int>(ErrorCode::MEMORY_ERROR) == 3);
        REQUIRE(static_cast<int>(ErrorCode::TYPE_ERROR) == 4);
        REQUIRE(static_cast<int>(ErrorCode::ARGUMENT_ERROR) == 5);
        REQUIRE(static_cast<int>(ErrorCode::STACK_OVERFLOW) == 6);
        REQUIRE(static_cast<int>(ErrorCode::COROUTINE_ERROR) == 7);
        REQUIRE(static_cast<int>(ErrorCode::IO_ERROR) == 8);
        REQUIRE(static_cast<int>(ErrorCode::UNKNOWN_ERROR) == 9);
    }
}

TEST_CASE("Exception class functionality", "[core][error]") {
    SECTION("Basic exception creation") {
        Exception ex("Test error", ErrorCode::RUNTIME_ERROR);
        
        REQUIRE(ex.what() == String("Test error"));
        REQUIRE(ex.code() == ErrorCode::RUNTIME_ERROR);
        REQUIRE_FALSE(ex.detailed_message().empty());
    }
    
    SECTION("Exception with source location") {
        auto location = std::source_location::current();
        Exception ex("Test error", ErrorCode::SYNTAX_ERROR, location);
        
        REQUIRE(ex.code() == ErrorCode::SYNTAX_ERROR);
        REQUIRE(ex.location().line() == location.line());
        REQUIRE(ex.location().file_name() == location.file_name());
    }
    
    SECTION("Detailed message formatting") {
        Exception ex("Test detailed error", ErrorCode::MEMORY_ERROR);
        String detailed = ex.detailed_message();
        
        REQUIRE(detailed.find("RangeLua Exception") != String::npos);
        REQUIRE(detailed.find("MEMORY_ERROR") != String::npos);
        REQUIRE(detailed.find("Test detailed error") != String::npos);
    }
}

TEST_CASE("Specialized exception types", "[core][error]") {
    SECTION("SyntaxError") {
        SourceLocation src_loc{"test.lua", 42, 10};
        SyntaxError syntax_err("Invalid syntax", src_loc);
        
        REQUIRE(syntax_err.code() == ErrorCode::SYNTAX_ERROR);
        REQUIRE(syntax_err.source_location().filename_ == "test.lua");
        REQUIRE(syntax_err.source_location().line_ == 42);
        REQUIRE(syntax_err.source_location().column_ == 10);
    }
    
    SECTION("RuntimeError") {
        RuntimeError runtime_err("Runtime failure");
        
        REQUIRE(runtime_err.code() == ErrorCode::RUNTIME_ERROR);
        REQUIRE(runtime_err.what() == String("Runtime failure"));
    }
    
    SECTION("MemoryError") {
        MemoryError mem_err("Out of memory", 1024);
        
        REQUIRE(mem_err.code() == ErrorCode::MEMORY_ERROR);
        REQUIRE(mem_err.requested_size() == 1024);
    }
    
    SECTION("TypeError") {
        TypeError type_err("Type mismatch", "number", "string");
        
        REQUIRE(type_err.code() == ErrorCode::TYPE_ERROR);
        REQUIRE(type_err.expected_type() == "number");
        REQUIRE(type_err.actual_type() == "string");
    }
    
    SECTION("StackOverflowError") {
        StackOverflowError stack_err(2048);
        
        REQUIRE(stack_err.code() == ErrorCode::STACK_OVERFLOW);
        REQUIRE(stack_err.stack_size() == 2048);
    }
}

TEST_CASE("ErrorCategory functionality", "[core][error]") {
    SECTION("Error category name") {
        const auto& category = ErrorCategory::instance();
        REQUIRE(category.name() == String("rangelua"));
    }
    
    SECTION("Error messages") {
        const auto& category = ErrorCategory::instance();
        
        REQUIRE(category.message(static_cast<int>(ErrorCode::SUCCESS)).find("success") != String::npos);
        REQUIRE(category.message(static_cast<int>(ErrorCode::SYNTAX_ERROR)).find("Syntax") != String::npos);
        REQUIRE(category.message(static_cast<int>(ErrorCode::MEMORY_ERROR)).find("Memory") != String::npos);
    }
}

TEST_CASE("Result type operations", "[core][error]") {
    SECTION("Success result") {
        auto result = make_success(42);
        
        REQUIRE(is_success(result));
        REQUIRE_FALSE(is_error(result));
        REQUIRE(get_value(result) == 42);
    }
    
    SECTION("Error result") {
        auto result = make_error<int>(ErrorCode::RUNTIME_ERROR);
        
        REQUIRE_FALSE(is_success(result));
        REQUIRE(is_error(result));
        REQUIRE(get_error(result) == ErrorCode::RUNTIME_ERROR);
    }
    
    SECTION("Status type") {
        auto success_status = make_success();
        auto error_status = make_error<std::monostate>(ErrorCode::IO_ERROR);
        
        REQUIRE(is_success(success_status));
        REQUIRE(is_error(error_status));
    }
}

TEST_CASE("Monadic operations", "[core][error]") {
    SECTION("and_then with success") {
        auto result = make_success(5);
        auto chained = and_then(result, [](int x) { return make_success(x * 2); });
        
        REQUIRE(is_success(chained));
        REQUIRE(get_value(chained) == 10);
    }
    
    SECTION("and_then with error") {
        auto result = make_error<int>(ErrorCode::RUNTIME_ERROR);
        auto chained = and_then(result, [](int x) { return make_success(x * 2); });
        
        REQUIRE(is_error(chained));
        REQUIRE(get_error(chained) == ErrorCode::RUNTIME_ERROR);
    }
    
    SECTION("or_else with error") {
        auto result = make_error<int>(ErrorCode::MEMORY_ERROR);
        auto recovered = or_else(result, [](ErrorCode) { return make_success(42); });
        
        REQUIRE(is_success(recovered));
        REQUIRE(get_value(recovered) == 42);
    }
    
    SECTION("transform with success") {
        auto result = make_success(3);
        auto transformed = transform(result, [](int x) { return x * x; });
        
        REQUIRE(is_success(transformed));
        REQUIRE(get_value(transformed) == 9);
    }
}

TEST_CASE("Error utility functions", "[core][error]") {
    SECTION("value_or with success") {
        auto result = make_success(100);
        REQUIRE(value_or(result, 0) == 100);
    }
    
    SECTION("value_or with error") {
        auto result = make_error<int>(ErrorCode::TYPE_ERROR);
        REQUIRE(value_or(result, 42) == 42);
    }
    
    SECTION("value_or_else") {
        auto result = make_error<int>(ErrorCode::ARGUMENT_ERROR);
        auto value = value_or_else(result, [](ErrorCode code) {
            return static_cast<int>(code) * 10;
        });
        REQUIRE(value == 50); // ARGUMENT_ERROR = 5, so 5 * 10 = 50
    }
}

TEST_CASE("Error formatting", "[core][error]") {
    SECTION("format_error_message") {
        auto msg = format_error_message(ErrorCode::SYNTAX_ERROR, "parser");
        REQUIRE(msg.find("SYNTAX_ERROR") != String::npos);
        REQUIRE(msg.find("parser") != String::npos);
    }
    
    SECTION("format_exception_details") {
        Exception ex("Test exception", ErrorCode::RUNTIME_ERROR);
        auto details = format_exception_details(ex);
        
        REQUIRE(details.find("Exception Details") != String::npos);
        REQUIRE(details.find("RUNTIME_ERROR") != String::npos);
        REQUIRE(details.find("Test exception") != String::npos);
    }
}

TEST_CASE("std::error_code integration", "[core][error]") {
    SECTION("make_error_code") {
        auto ec = make_error_code(ErrorCode::MEMORY_ERROR);
        
        REQUIRE(ec.value() == static_cast<int>(ErrorCode::MEMORY_ERROR));
        REQUIRE(ec.category().name() == String("rangelua"));
    }
}
