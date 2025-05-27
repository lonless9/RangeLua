/**
 * @file test_profiler.cpp
 * @brief Test cases for profiler system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <rangelua/utils/profiler.hpp>

#include <thread>
#include <chrono>

using namespace rangelua;
using namespace rangelua::utils;

TEST_CASE("PerformanceMetrics functionality", "[utils][profiler]") {
    SECTION("Initial state") {
        PerformanceMetrics metrics;

        REQUIRE(metrics.total_time.count() == 0);
        REQUIRE(metrics.call_count == 0);
        REQUIRE(metrics.memory_allocated == 0);
        REQUIRE(metrics.memory_deallocated == 0);
    }

    SECTION("Update with timing") {
        PerformanceMetrics metrics;

        auto duration1 = std::chrono::nanoseconds(1000000); // 1ms
        auto duration2 = std::chrono::nanoseconds(2000000); // 2ms

        metrics.update(duration1);
        REQUIRE(metrics.call_count == 1);
        REQUIRE(metrics.total_time == duration1);
        REQUIRE(metrics.min_time == duration1);
        REQUIRE(metrics.max_time == duration1);
        REQUIRE(metrics.avg_time == duration1);

        metrics.update(duration2);
        REQUIRE(metrics.call_count == 2);
        REQUIRE(metrics.total_time == duration1 + duration2);
        REQUIRE(metrics.min_time == duration1);
        REQUIRE(metrics.max_time == duration2);
        REQUIRE(metrics.avg_time == (duration1 + duration2) / 2);
    }

    SECTION("Update with memory") {
        PerformanceMetrics metrics;

        metrics.update(std::chrono::nanoseconds(0), 1024);  // Allocation
        REQUIRE(metrics.memory_allocated == 1024);
        REQUIRE(metrics.memory_deallocated == 0);

        metrics.update(std::chrono::nanoseconds(0), -512); // Deallocation
        REQUIRE(metrics.memory_allocated == 1024);
        REQUIRE(metrics.memory_deallocated == 512);
    }

    SECTION("to_string formatting") {
        PerformanceMetrics metrics;
        metrics.update(std::chrono::nanoseconds(1000000), 1024);

        String str = metrics.to_string();
        REQUIRE(str.find("Calls: 1") != String::npos);
        REQUIRE(str.find("1.000ms") != String::npos);
    }
}

TEST_CASE("Profiler basic functionality", "[utils][profiler]") {
    SECTION("Enable/disable profiling") {
        Profiler::set_enabled(false);
        REQUIRE_FALSE(Profiler::is_enabled());

        Profiler::set_enabled(true);
        REQUIRE(Profiler::is_enabled());
    }

    SECTION("Clear profiling data") {
        Profiler::clear();
        auto metrics = Profiler::get_all_metrics();
        REQUIRE(metrics.empty());
    }
}

TEST_CASE("Profiler timing functionality", "[utils][profiler]") {
    SECTION("Basic timing") {
        Profiler::clear();
        Profiler::set_enabled(true);

        Profiler::start("test_function");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Profiler::end("test_function");

        auto metrics = Profiler::get_metrics("test_function");
        REQUIRE(metrics.has_value());
        REQUIRE(metrics->call_count == 1);
        REQUIRE(metrics->total_time.count() > 5000000); // At least 5ms
    }

    SECTION("Multiple calls") {
        Profiler::clear();
        Profiler::set_enabled(true);

        for (int i = 0; i < 3; ++i) {
            Profiler::start("repeated_function");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            Profiler::end("repeated_function");
        }

        auto metrics = Profiler::get_metrics("repeated_function");
        REQUIRE(metrics.has_value());
        REQUIRE(metrics->call_count == 3);
        REQUIRE(metrics->total_time.count() > 10000000); // At least 10ms total
    }

    SECTION("Non-existent function") {
        Profiler::clear();

        auto metrics = Profiler::get_metrics("non_existent");
        REQUIRE_FALSE(metrics.has_value());
    }
}

TEST_CASE("Profiler memory tracking", "[utils][profiler]") {
    SECTION("Record allocations") {
        Profiler::clear();
        Profiler::set_enabled(true);

        Profiler::record_allocation("test_context", 1024);
        Profiler::record_allocation("test_context", 512);

        auto metrics = Profiler::get_metrics("test_context");
        REQUIRE(metrics.has_value());
        REQUIRE(metrics->memory_allocated == 1536);
    }

    SECTION("Record deallocations") {
        Profiler::clear();
        Profiler::set_enabled(true);

        Profiler::record_allocation("test_context", 1024);
        Profiler::record_deallocation("test_context", 512);

        auto metrics = Profiler::get_metrics("test_context");
        REQUIRE(metrics.has_value());
        REQUIRE(metrics->memory_allocated == 1024);
        REQUIRE(metrics->memory_deallocated == 512);
    }
}

TEST_CASE("ScopedProfiler RAII functionality", "[utils][profiler]") {
    SECTION("Automatic timing") {
        Profiler::clear();
        Profiler::set_enabled(true);

        {
            ScopedProfiler prof("scoped_test");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } // Profiler should automatically end here

        auto metrics = Profiler::get_metrics("scoped_test");
        REQUIRE(metrics.has_value());
        REQUIRE(metrics->call_count == 1);
        REQUIRE(metrics->total_time.count() > 2000000); // At least 2ms
    }

    SECTION("Disabled profiling") {
        Profiler::clear();
        Profiler::set_enabled(false);

        {
            ScopedProfiler prof("disabled_test");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        auto metrics = Profiler::get_metrics("disabled_test");
        REQUIRE_FALSE(metrics.has_value());

        Profiler::set_enabled(true); // Reset for other tests
    }
}

TEST_CASE("MemoryProfiler functionality", "[utils][profiler]") {
    SECTION("Record allocation and deallocation") {
        MemoryProfiler::clear();

        void* ptr1 = reinterpret_cast<void*>(0x1000);
        void* ptr2 = reinterpret_cast<void*>(0x2000);

        MemoryProfiler::record_allocation(ptr1, 1024, "test_context");
        MemoryProfiler::record_allocation(ptr2, 512, "test_context");

        REQUIRE(MemoryProfiler::get_current_usage() == 1536);
        REQUIRE(MemoryProfiler::get_allocation_count() == 2);

        MemoryProfiler::record_deallocation(ptr1);
        REQUIRE(MemoryProfiler::get_current_usage() == 512);

        MemoryProfiler::record_deallocation(ptr2);
        REQUIRE(MemoryProfiler::get_current_usage() == 0);
    }

    SECTION("Peak usage tracking") {
        MemoryProfiler::clear();

        void* ptr1 = reinterpret_cast<void*>(0x1000);
        void* ptr2 = reinterpret_cast<void*>(0x2000);

        MemoryProfiler::record_allocation(ptr1, 1024);
        REQUIRE(MemoryProfiler::get_peak_usage() == 1024);

        MemoryProfiler::record_allocation(ptr2, 512);
        REQUIRE(MemoryProfiler::get_peak_usage() == 1536);

        MemoryProfiler::record_deallocation(ptr1);
        REQUIRE(MemoryProfiler::get_peak_usage() == 1536); // Peak should remain
        REQUIRE(MemoryProfiler::get_current_usage() == 512);
    }

    SECTION("Generate memory report") {
        MemoryProfiler::clear();

        void* ptr = reinterpret_cast<void*>(0x1000);
        MemoryProfiler::record_allocation(ptr, 1024, "test_allocation");

        String report = MemoryProfiler::generate_report();
        REQUIRE(report.find("Memory Usage Report") != String::npos);
        REQUIRE(report.find("1.00 KB") != String::npos); // Should be formatted as KB
        REQUIRE(report.find("test_allocation") != String::npos);
    }
}

TEST_CASE("Profiler report generation", "[utils][profiler]") {
    SECTION("Generate performance report") {
        Profiler::clear();
        Profiler::set_enabled(true);

        // Add some test data
        Profiler::start("function_a");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Profiler::end("function_a");

        Profiler::start("function_b");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        Profiler::end("function_b");

        String report = Profiler::generate_report();
        REQUIRE(report.find("Performance Report") != String::npos);
        REQUIRE(report.find("function_a") != String::npos);
        REQUIRE(report.find("function_b") != String::npos);
        REQUIRE(report.find("Total Functions: 2") != String::npos);
    }

    SECTION("Empty report") {
        Profiler::clear();

        String report = Profiler::generate_report();
        REQUIRE(report.find("No profiling data available") != String::npos);
    }
}

TEST_CASE("Profiler macros", "[utils][profiler]") {
    SECTION("RANGELUA_PROFILE macro") {
        Profiler::clear();
        Profiler::set_enabled(true);

        {
            RANGELUA_PROFILE("macro_test");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        auto metrics = Profiler::get_metrics("macro_test");
        if constexpr (config::DEBUG_ENABLED) {
            REQUIRE(metrics.has_value());
            REQUIRE(metrics->call_count == 1);
        }
    }

    SECTION("RANGELUA_PROFILE_FUNCTION macro") {
        Profiler::clear();
        Profiler::set_enabled(true);

        auto test_function = []() {
            RANGELUA_PROFILE_FUNCTION();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        };

        test_function();

        // In debug mode, should have profiling data
        if constexpr (config::DEBUG_ENABLED) {
            auto all_metrics = Profiler::get_all_metrics();
            REQUIRE_FALSE(all_metrics.empty());
        }
    }
}

TEST_CASE("PerformanceMonitor functionality", "[utils][profiler]") {
    SECTION("Start and stop monitoring") {
        REQUIRE_FALSE(PerformanceMonitor::is_monitoring());

        bool callback_called = false;
        auto callback = [&callback_called](const auto& metrics) {
            callback_called = true;
        };

        PerformanceMonitor::start_monitoring(std::chrono::milliseconds(10), callback);
        REQUIRE(PerformanceMonitor::is_monitoring());

        // Wait a bit for callback
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        PerformanceMonitor::stop_monitoring();
        REQUIRE_FALSE(PerformanceMonitor::is_monitoring());

        // Callback should have been called at least once
        REQUIRE(callback_called);
    }
}

// Helper function to test function profiling
void test_function_profiling() {
    RANGELUA_PROFILE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

TEST_CASE("Thread safety", "[utils][profiler]") {
    SECTION("Multiple threads profiling") {
        Profiler::clear();
        Profiler::set_enabled(true);

        constexpr int num_threads = 4;
        std::vector<std::thread> threads;

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([i]() {
                String name = "thread_function_" + std::to_string(i);

                for (int j = 0; j < 5; ++j) {
                    Profiler::start(name);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    Profiler::end(name);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        // Check that all threads recorded their metrics
        auto all_metrics = Profiler::get_all_metrics();
        REQUIRE(all_metrics.size() == num_threads);

        for (int i = 0; i < num_threads; ++i) {
            String name = "thread_function_" + std::to_string(i);
            auto metrics = Profiler::get_metrics(name);
            REQUIRE(metrics.has_value());
            REQUIRE(metrics->call_count == 5);
        }
    }
}
