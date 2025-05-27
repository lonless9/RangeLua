/**
 * @file test_error_debug_integration.cpp
 * @brief Integration tests for error and debug module takeover of runtime
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/runtime/vm.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/memory.hpp>
#include <rangelua/core/error.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

using namespace rangelua;
using namespace rangelua::utils;
// Use explicit namespace qualifiers to avoid ambiguity
using rangelua::runtime::VirtualMachine;
using rangelua::runtime::VMState;
using rangelua::runtime::getMemoryManager;
using rangelua::runtime::getGarbageCollector;

TEST_CASE("Error module integration with runtime", "[integration][error][runtime]") {
    SECTION("Memory manager error handling") {
        // Test that memory manager returns Result types instead of throwing
        auto memory_result = getMemoryManager();
        REQUIRE(is_success(memory_result));

        auto gc_result = getGarbageCollector();
        REQUIRE(is_success(gc_result));
    }

    SECTION("VM error handling with debug integration") {
        VirtualMachine vm;

        // Test error setting with debug information
        vm.trigger_runtime_error("Test runtime error");
        REQUIRE(vm.state() == VMState::Error);
        REQUIRE(vm.last_error() == ErrorCode::RUNTIME_ERROR);
    }

    SECTION("Value arithmetic error handling") {
        rangelua::runtime::Value str_val("hello");
        rangelua::runtime::Value num_val(42.0);

        // This should trigger error logging but not crash
        rangelua::runtime::Value result = str_val + num_val;
        REQUIRE(result.is_nil());
    }
}

TEST_CASE("Debug module integration with runtime", "[integration][debug][runtime]") {
    SECTION("Debug assertions in runtime") {
        // Test that debug assertions work in runtime context
        RANGELUA_ASSERT(true);  // Should not trigger

        // Test debug printing
        RANGELUA_DEBUG_PRINT("Test debug message from runtime integration");
    }

    SECTION("Debug timer integration") {
        RANGELUA_DEBUG_TIMER("test_timer");

        // Simulate some work
        rangelua::runtime::Value val1(1.0);
        rangelua::runtime::Value val2(2.0);
        rangelua::runtime::Value result = val1 + val2;

        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(is_success(num_result));
        REQUIRE(get_value(num_result) == 3.0);
    }

    SECTION("Memory debugging") {
        auto memory_result = getMemoryManager();
        REQUIRE(is_success(memory_result));

        auto* manager = get_value(memory_result);
        REQUIRE(manager != nullptr);

        // Test memory size formatting
        String formatted = RANGELUA_FORMAT_MEMORY(1024 * 1024);
        REQUIRE_FALSE(formatted.empty());
    }
}

TEST_CASE("Error recovery integration", "[integration][error][recovery]") {
    SECTION("Result type error propagation") {
        // Test that Result types properly propagate errors
        auto test_function = []() -> Result<int> {
            return ErrorCode::RUNTIME_ERROR;
        };

        auto result = test_function();
        REQUIRE(is_error(result));
        REQUIRE(get_error(result) == ErrorCode::RUNTIME_ERROR);
    }

    SECTION("Error transformation") {
        Result<int> error_result = ErrorCode::TYPE_ERROR;

        auto transformed = transform_error(error_result, [](ErrorCode code) {
            return ErrorCode::RUNTIME_ERROR;
        });

        REQUIRE(is_error(transformed));
        REQUIRE(get_error(transformed) == ErrorCode::RUNTIME_ERROR);
    }

    SECTION("Value or default") {
        Result<int> success_result = 42;
        Result<int> error_result = ErrorCode::RUNTIME_ERROR;

        REQUIRE(value_or(success_result, 0) == 42);
        REQUIRE(value_or(error_result, 0) == 0);
    }
}

TEST_CASE("Thread-safe memory management", "[integration][memory][threading]") {
    SECTION("Thread-local memory managers") {
        // Test that each thread gets its own memory manager
        auto memory_result1 = getMemoryManager();
        auto memory_result2 = getMemoryManager();

        REQUIRE(is_success(memory_result1));
        REQUIRE(is_success(memory_result2));

        // Should be the same instance within the same thread
        REQUIRE(get_value(memory_result1) == get_value(memory_result2));
    }

    SECTION("Thread-local garbage collectors") {
        auto gc_result1 = getGarbageCollector();
        auto gc_result2 = getGarbageCollector();

        REQUIRE(is_success(gc_result1));
        REQUIRE(is_success(gc_result2));

        // Should be the same instance within the same thread
        REQUIRE(get_value(gc_result1) == get_value(gc_result2));
    }
}

TEST_CASE("Enhanced error reporting", "[integration][error][reporting]") {
    SECTION("Exception with source location") {
        try {
            throw RuntimeError("Test runtime error");
        } catch (const Exception& e) {
            REQUIRE(e.code() == ErrorCode::RUNTIME_ERROR);
            REQUIRE_FALSE(e.detailed_message().empty());

            // Should contain source location information
            String detailed = e.detailed_message();
            REQUIRE(detailed.find("test_error_debug_integration.cpp") != String::npos);
        }
    }

    SECTION("Error code to string conversion") {
        REQUIRE(error_code_to_string(ErrorCode::SUCCESS) == "SUCCESS");
        REQUIRE(error_code_to_string(ErrorCode::RUNTIME_ERROR) == "RUNTIME_ERROR");
        REQUIRE(error_code_to_string(ErrorCode::TYPE_ERROR) == "TYPE_ERROR");
        REQUIRE(error_code_to_string(ErrorCode::MEMORY_ERROR) == "MEMORY_ERROR");
    }

    SECTION("Error message formatting") {
        String formatted = format_error_message(ErrorCode::TYPE_ERROR, "value conversion");
        REQUIRE_FALSE(formatted.empty());
        REQUIRE(formatted.find("TYPE_ERROR") != String::npos);
        REQUIRE(formatted.find("value conversion") != String::npos);
    }
}
