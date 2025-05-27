/**
 * @file test_error_recovery.cpp
 * @brief Test cases for error recovery system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <rangelua/utils/error_recovery.hpp>
#include <rangelua/core/error.hpp>

#include <thread>
#include <chrono>

using namespace rangelua;
using namespace rangelua::utils;

// Test retry strategy
class TestRetryStrategy : public RetryStrategy<int> {
public:
    TestRetryStrategy(Size max_attempts = 3) : RetryStrategy<int>(max_attempts) {}

protected:
    bool attempt_recovery() override {
        attempt_count_++;
        return attempt_count_ >= 2; // Succeed on second attempt
    }

    int create_default_value() override {
        return 42;
    }

private:
    Size attempt_count_ = 0;
};

TEST_CASE("FallbackStrategy functionality", "[utils][error_recovery]") {
    SECTION("Successful fallback") {
        FallbackStrategy<int> strategy(100);
        
        auto result = strategy.recover(ErrorCode::RUNTIME_ERROR, "test");
        REQUIRE(result.has_value());
        REQUIRE(*result == 100);
    }
    
    SECTION("Cannot handle critical errors") {
        FallbackStrategy<int> strategy(100);
        
        auto result = strategy.recover(ErrorCode::STACK_OVERFLOW, "test");
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Can handle most errors") {
        FallbackStrategy<int> strategy(100);
        
        REQUIRE(strategy.can_handle(ErrorCode::RUNTIME_ERROR));
        REQUIRE(strategy.can_handle(ErrorCode::MEMORY_ERROR));
        REQUIRE(strategy.can_handle(ErrorCode::TYPE_ERROR));
        REQUIRE_FALSE(strategy.can_handle(ErrorCode::STACK_OVERFLOW));
    }
}

TEST_CASE("RetryStrategy functionality", "[utils][error_recovery]") {
    SECTION("Successful retry") {
        TestRetryStrategy strategy(3);
        
        auto result = strategy.recover(ErrorCode::IO_ERROR, "test");
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }
    
    SECTION("Can handle transient errors") {
        TestRetryStrategy strategy;
        
        REQUIRE(strategy.can_handle(ErrorCode::IO_ERROR));
        REQUIRE(strategy.can_handle(ErrorCode::MEMORY_ERROR));
        REQUIRE(strategy.can_handle(ErrorCode::COROUTINE_ERROR));
        REQUIRE_FALSE(strategy.can_handle(ErrorCode::SYNTAX_ERROR));
    }
}

TEST_CASE("CircuitBreaker functionality", "[utils][error_recovery]") {
    SECTION("Initial state is CLOSED") {
        CircuitBreaker breaker(3);
        REQUIRE(breaker.get_state() == CircuitBreaker::State::CLOSED);
        REQUIRE(breaker.get_failure_count() == 0);
    }
    
    SECTION("Successful execution") {
        CircuitBreaker breaker(3);
        
        auto result = breaker.execute([]() { return 42; });
        REQUIRE(result == 42);
        REQUIRE(breaker.get_state() == CircuitBreaker::State::CLOSED);
    }
    
    SECTION("Circuit opens after threshold failures") {
        CircuitBreaker breaker(2); // Low threshold for testing
        
        // First failure
        REQUIRE_THROWS(breaker.execute([]() -> int { 
            throw RuntimeError("Test error"); 
        }));
        REQUIRE(breaker.get_state() == CircuitBreaker::State::CLOSED);
        REQUIRE(breaker.get_failure_count() == 1);
        
        // Second failure - should open circuit
        REQUIRE_THROWS(breaker.execute([]() -> int { 
            throw RuntimeError("Test error"); 
        }));
        REQUIRE(breaker.get_state() == CircuitBreaker::State::OPEN);
        REQUIRE(breaker.get_failure_count() == 2);
        
        // Third attempt should fail immediately
        REQUIRE_THROWS_AS(breaker.execute([]() { return 42; }), RuntimeError);
    }
}

TEST_CASE("ErrorRecoveryManager functionality", "[utils][error_recovery]") {
    SECTION("Recovery with fallback strategy") {
        ErrorRecoveryManager<int> manager;
        manager.add_strategy(std::make_unique<FallbackStrategy<int>>(999));
        
        auto result = manager.attempt_recovery(ErrorCode::RUNTIME_ERROR, "test");
        REQUIRE(result.has_value());
        REQUIRE(*result == 999);
    }
    
    SECTION("Recovery with retry strategy") {
        ErrorRecoveryManager<int> manager;
        manager.add_strategy(std::make_unique<TestRetryStrategy>(3));
        
        auto result = manager.attempt_recovery(ErrorCode::IO_ERROR, "test");
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }
    
    SECTION("Multiple strategies - first match wins") {
        ErrorRecoveryManager<int> manager;
        manager.add_strategy(std::make_unique<FallbackStrategy<int>>(100));
        manager.add_strategy(std::make_unique<TestRetryStrategy>(3));
        
        auto result = manager.attempt_recovery(ErrorCode::RUNTIME_ERROR, "test");
        REQUIRE(result.has_value());
        REQUIRE(*result == 100); // Fallback strategy should be used first
    }
    
    SECTION("Execute with recovery") {
        ErrorRecoveryManager<int> manager;
        manager.add_strategy(std::make_unique<FallbackStrategy<int>>(777));
        
        // Successful operation
        auto result1 = manager.execute_with_recovery([]() { return 123; });
        REQUIRE(is_success(result1));
        REQUIRE(get_value(result1) == 123);
        
        // Failed operation with recovery
        auto result2 = manager.execute_with_recovery([]() -> int {
            throw RuntimeError("Test error");
        });
        REQUIRE(is_success(result2));
        REQUIRE(get_value(result2) == 777);
    }
}

TEST_CASE("ErrorContext functionality", "[utils][error_recovery]") {
    SECTION("Single context") {
        {
            ErrorContext ctx("test_function");
            REQUIRE(ErrorContext::get_current_context() == "test_function");
        }
        REQUIRE(ErrorContext::get_current_context().empty());
    }
    
    SECTION("Nested contexts") {
        {
            ErrorContext ctx1("outer_function");
            REQUIRE(ErrorContext::get_current_context() == "outer_function");
            
            {
                ErrorContext ctx2("inner_function");
                REQUIRE(ErrorContext::get_current_context() == "outer_function -> inner_function");
                
                {
                    ErrorContext ctx3("deep_function");
                    REQUIRE(ErrorContext::get_current_context() == 
                           "outer_function -> inner_function -> deep_function");
                }
                
                REQUIRE(ErrorContext::get_current_context() == "outer_function -> inner_function");
            }
            
            REQUIRE(ErrorContext::get_current_context() == "outer_function");
        }
        REQUIRE(ErrorContext::get_current_context().empty());
    }
    
    SECTION("Thread-local context") {
        std::string main_context;
        std::string thread_context;
        
        {
            ErrorContext ctx("main_thread");
            main_context = ErrorContext::get_current_context();
            
            std::thread t([&thread_context]() {
                ErrorContext ctx("worker_thread");
                thread_context = ErrorContext::get_current_context();
            });
            t.join();
        }
        
        REQUIRE(main_context == "main_thread");
        REQUIRE(thread_context == "worker_thread");
        REQUIRE(ErrorContext::get_current_context().empty());
    }
}

// Helper function to test error context macros
void test_function_with_context() {
    RANGELUA_ERROR_CONTEXT_FUNC();
    
    // Context should include function name
    auto context = ErrorContext::get_current_context();
    REQUIRE(context.find("test_function_with_context") != String::npos);
}

TEST_CASE("Error context macros", "[utils][error_recovery]") {
    SECTION("RANGELUA_ERROR_CONTEXT macro") {
        {
            RANGELUA_ERROR_CONTEXT("test_macro");
            REQUIRE(ErrorContext::get_current_context() == "test_macro");
        }
        REQUIRE(ErrorContext::get_current_context().empty());
    }
    
    SECTION("RANGELUA_ERROR_CONTEXT_FUNC macro") {
        test_function_with_context();
        REQUIRE(ErrorContext::get_current_context().empty());
    }
}

TEST_CASE("Integration test - comprehensive error handling", "[utils][error_recovery]") {
    SECTION("Complete error handling workflow") {
        ErrorRecoveryManager<String> manager;
        manager.add_strategy(std::make_unique<FallbackStrategy<String>>("fallback_value"));
        
        CircuitBreaker breaker(2);
        
        auto risky_operation = [&breaker]() -> String {
            return breaker.execute([]() -> String {
                static int call_count = 0;
                call_count++;
                
                if (call_count <= 2) {
                    throw RuntimeError("Simulated failure");
                }
                return "success";
            });
        };
        
        // First two calls should fail and trigger recovery
        {
            RANGELUA_ERROR_CONTEXT("integration_test");
            
            auto result1 = manager.execute_with_recovery(risky_operation);
            REQUIRE(is_success(result1));
            REQUIRE(get_value(result1) == "fallback_value");
            
            auto result2 = manager.execute_with_recovery(risky_operation);
            REQUIRE(is_success(result2));
            REQUIRE(get_value(result2) == "fallback_value");
            
            // Circuit should be open now, so third call should fail immediately
            REQUIRE_THROWS(risky_operation());
        }
    }
}
