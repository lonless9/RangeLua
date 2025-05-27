/**
 * @file test_debug.cpp
 * @brief Test cases for debug utilities
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>
#include <rangelua/core/config.hpp>

#include <thread>
#include <chrono>

using namespace rangelua;
using namespace rangelua::utils;

TEST_CASE("Debug class basic functionality", "[utils][debug]") {
    SECTION("is_enabled returns correct value") {
        REQUIRE(Debug::is_enabled() == config::DEBUG_ENABLED);
        REQUIRE(Debug::is_trace_enabled() == config::TRACE_ENABLED);
    }

    SECTION("print function works") {
        // This test mainly ensures the function doesn't crash
        // In debug mode, it should print; in release mode, it should be optimized out
        REQUIRE_NOTHROW(Debug::print("Test debug message"));
    }

    SECTION("assert_msg with true condition") {
        // Should not throw or abort
        REQUIRE_NOTHROW(Debug::assert_msg(true, "This should not trigger"));
    }
}

TEST_CASE("Debug thread naming", "[utils][debug]") {
    SECTION("set_thread_name") {
        REQUIRE_NOTHROW(Debug::set_thread_name("TestThread"));

        // Test with different thread
        std::thread t([]() {
            Debug::set_thread_name("WorkerThread");
            Debug::print("Message from worker thread");
        });
        t.join();
    }
}

TEST_CASE("Debug timing functionality", "[utils][debug]") {
    SECTION("start_timer and end_timer") {
        Debug::start_timer("test_timer");

        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto duration = Debug::end_timer("test_timer");

        // Should have measured some time (at least 5ms, allowing for timing variance)
        REQUIRE(duration.count() > 5000000); // 5ms in nanoseconds
    }

    SECTION("end_timer with non-existent timer") {
        auto duration = Debug::end_timer("non_existent_timer");
        REQUIRE(duration.count() == 0);
    }

    SECTION("multiple timers") {
        Debug::start_timer("timer1");
        Debug::start_timer("timer2");

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        auto duration1 = Debug::end_timer("timer1");
        auto duration2 = Debug::end_timer("timer2");

        REQUIRE(duration1.count() > 0);
        REQUIRE(duration2.count() > 0);
    }
}

TEST_CASE("Memory size formatting", "[utils][debug]") {
    SECTION("format_memory_size with bytes") {
        auto result = Debug::format_memory_size(512);
        REQUIRE(result.find("512") != String::npos);
        REQUIRE(result.find("bytes") != String::npos);
    }

    SECTION("format_memory_size with KB") {
        auto result = Debug::format_memory_size(2048);
        REQUIRE(result.find("2.00") != String::npos);
        REQUIRE(result.find("KB") != String::npos);
    }

    SECTION("format_memory_size with MB") {
        auto result = Debug::format_memory_size(5 * 1024 * 1024);
        REQUIRE(result.find("5.00") != String::npos);
        REQUIRE(result.find("MB") != String::npos);
    }

    SECTION("format_memory_size with GB") {
        auto result = Debug::format_memory_size(3ULL * 1024 * 1024 * 1024);
        REQUIRE(result.find("3.00") != String::npos);
        REQUIRE(result.find("GB") != String::npos);
    }
}

TEST_CASE("Stack trace functionality", "[utils][debug]") {
    SECTION("dump_stack_trace") {
        // This test mainly ensures the function doesn't crash
        REQUIRE_NOTHROW(Debug::dump_stack_trace());
    }
}

TEST_CASE("DebugTimer RAII class", "[utils][debug]") {
    SECTION("DebugTimer automatic timing") {
        auto start_time = std::chrono::high_resolution_clock::now();

        {
            DebugTimer timer("raii_test");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } // Timer should automatically end here

        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        // Should have taken at least 5ms
        REQUIRE(total_duration.count() >= 5);
    }

    SECTION("DebugTimer is non-copyable and non-movable") {
        // These should not compile:
        // DebugTimer timer1("test");
        // DebugTimer timer2 = timer1;        // Copy constructor
        // DebugTimer timer3 = std::move(timer1); // Move constructor
        // timer2 = timer1;                   // Copy assignment
        // timer3 = std::move(timer1);        // Move assignment

        // This test just ensures the class can be instantiated
        {
            DebugTimer timer("test");
            REQUIRE(true); // Just to have a test assertion
        }
    }
}

TEST_CASE("Debug macros", "[utils][debug]") {
    SECTION("RANGELUA_ASSERT with true condition") {
        // These should not crash or throw
        RANGELUA_ASSERT(true);
        RANGELUA_ASSERT(1 == 1);
        RANGELUA_ASSERT(2 + 2 == 4);
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_ASSERT_MSG with true condition") {
        // These should not crash or throw
        RANGELUA_ASSERT_MSG(true, "This should not trigger");
        RANGELUA_ASSERT_MSG(5 > 3, "Math should work");
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_DEBUG_PRINT") {
        // This should not crash
        RANGELUA_DEBUG_PRINT("Test debug print");
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_DEBUG_TIMER") {
        {
            RANGELUA_DEBUG_TIMER("macro_timer");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Timer should automatically end when going out of scope
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_TRACE_FUNCTION") {
        RANGELUA_TRACE_FUNCTION();
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_DUMP_STACK") {
        RANGELUA_DUMP_STACK();
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_SET_THREAD_NAME") {
        RANGELUA_SET_THREAD_NAME("TestThread");
        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_FORMAT_MEMORY") {
        auto result = RANGELUA_FORMAT_MEMORY(1024);
        if constexpr (config::DEBUG_ENABLED) {
            REQUIRE_FALSE(result.empty());
        } else {
            REQUIRE(true); // Just to have a test assertion
        }
    }
}

TEST_CASE("Conditional debug macros", "[utils][debug]") {
    SECTION("RANGELUA_DEBUG_IF") {
        // These should not crash regardless of debug mode
        RANGELUA_DEBUG_IF(true, "Condition is true");
        RANGELUA_DEBUG_IF(false, "This should not print");
        RANGELUA_DEBUG_IF(2 + 2 == 4, "Math works");

        REQUIRE(true); // Just to have a test assertion
    }

    SECTION("RANGELUA_TRACE_IF") {
        RANGELUA_TRACE_IF(true, "Trace message");
        RANGELUA_TRACE_IF(false, "This trace should not print");

        REQUIRE(true); // Just to have a test assertion
    }
}

TEST_CASE("Thread safety", "[utils][debug]") {
    SECTION("Multiple threads using debug utilities") {
        constexpr int num_threads = 4;
        std::vector<std::thread> threads;

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([i]() {
                Debug::set_thread_name("Thread-" + std::to_string(i));
                Debug::start_timer("thread_timer_" + std::to_string(i));

                std::this_thread::sleep_for(std::chrono::milliseconds(5));

                Debug::print("Message from thread " + std::to_string(i));
                auto duration = Debug::end_timer("thread_timer_" + std::to_string(i));

                REQUIRE(duration.count() > 0);
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }
}

// Helper function to test function tracing
void test_function_with_trace() {
    RANGELUA_TRACE_FUNCTION();
    RANGELUA_DEBUG_PRINT("Inside test function");
}

TEST_CASE("Function tracing", "[utils][debug]") {
    SECTION("Function trace macro") {
        REQUIRE_NOTHROW(test_function_with_trace());
    }
}
