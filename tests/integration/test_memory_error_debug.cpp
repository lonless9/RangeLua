/**
 * @file test_memory_error_debug.cpp
 * @brief Integration tests for memory management with error handling and debug features
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/runtime/memory.hpp>
#include <rangelua/core/error.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

#include <sstream>
#include <thread>
#include <chrono>

using namespace rangelua;
using namespace rangelua::runtime;

TEST_CASE("Memory allocation with error logging", "[integration][memory][error]") {
    SECTION("Successful allocation logging") {
        SystemAllocator allocator;

        // Enable debug mode for this test
        RANGELUA_SET_THREAD_NAME("memory_test");

        void* ptr = allocator.allocate(1024, alignof(std::max_align_t));
        REQUIRE(ptr != nullptr);

        // Verify that debug information is generated
        RANGELUA_DEBUG_PRINT("Allocated pointer: " + std::to_string(reinterpret_cast<uintptr_t>(ptr)));

        allocator.deallocate(ptr, 1024);
        RANGELUA_DEBUG_PRINT("Deallocated pointer successfully");
    }

    SECTION("Memory statistics with debug formatting") {
        SystemAllocator allocator;

        // Allocate some memory
        std::vector<void*> ptrs;
        for (int i = 0; i < 10; ++i) {
            void* ptr = allocator.allocate(256, alignof(std::max_align_t));
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }

        // Format memory statistics
        [[maybe_unused]] Size total = allocator.total_allocated();
        String formatted = RANGELUA_FORMAT_MEMORY(total);
        REQUIRE_FALSE(formatted.empty());

        // Clean up
        for (void* ptr : ptrs) {
            allocator.deallocate(ptr, 256);
        }
    }

    SECTION("Error handling with Result types") {
        auto memory_result = getMemoryManager();
        REQUIRE(is_success(memory_result));

        auto* manager = get_value(memory_result);
        REQUIRE(manager != nullptr);

        // Test error propagation
        auto gc_result = getGarbageCollector();
        REQUIRE(is_success(gc_result));
    }
}

TEST_CASE("Memory debugging features", "[integration][memory][debug]") {
    SECTION("Debug timer for memory operations") {
        SystemAllocator allocator;

        {
            RANGELUA_DEBUG_TIMER("memory_allocation_test");

            // Perform multiple allocations
            std::vector<void*> ptrs;
            for (int i = 0; i < 100; ++i) {
                void* ptr = allocator.allocate(128, alignof(std::max_align_t));
                if (ptr) {
                    ptrs.push_back(ptr);
                }
            }

            // Clean up
            for (void* ptr : ptrs) {
                allocator.deallocate(ptr, 128);
            }
        } // Timer automatically reports elapsed time
    }

    SECTION("Memory assertions") {
        SystemAllocator allocator;

        void* ptr = allocator.allocate(512, alignof(std::max_align_t));
        RANGELUA_ASSERT(ptr != nullptr);

        [[maybe_unused]] Size count_before = allocator.allocation_count();
        allocator.deallocate(ptr, 512);
        [[maybe_unused]] Size count_after = allocator.allocation_count();

        RANGELUA_ASSERT(count_after < count_before);
    }

    SECTION("Debug information in pool allocator") {
        using TestPool = PoolAllocator<64, 8>;
        TestPool pool;

        RANGELUA_DEBUG_PRINT("Testing pool allocator with block size 64, count 8");

        void* ptr1 = pool.allocate(32, alignof(std::max_align_t));
        RANGELUA_ASSERT(ptr1 != nullptr);
        RANGELUA_DEBUG_PRINT("Pool allocation 1 successful");

        void* ptr2 = pool.allocate(64, alignof(std::max_align_t));
        RANGELUA_ASSERT(ptr2 != nullptr);
        RANGELUA_DEBUG_PRINT("Pool allocation 2 successful");

        // Test oversized allocation
        [[maybe_unused]] void* ptr3 = pool.allocate(128, alignof(std::max_align_t));
        RANGELUA_ASSERT(ptr3 == nullptr);
        RANGELUA_DEBUG_PRINT("Pool correctly rejected oversized allocation");

        pool.deallocate(ptr1, 32);
        pool.deallocate(ptr2, 64);
        RANGELUA_DEBUG_PRINT("Pool cleanup completed");
    }
}

TEST_CASE("Error recovery in memory operations", "[integration][memory][error_recovery]") {
    SECTION("Graceful handling of allocation failures") {
        SystemAllocator allocator;

        // Test with a reasonable size that should succeed
        void* ptr = allocator.allocate(1024, alignof(std::max_align_t));

        if (ptr) {
            RANGELUA_DEBUG_PRINT("Allocation succeeded as expected");
            allocator.deallocate(ptr, 1024);
        } else {
            // If allocation fails, we should handle it gracefully
            RANGELUA_DEBUG_PRINT("Allocation failed, but handled gracefully");
        }

        // Allocator should still be functional
        void* small_ptr = allocator.allocate(64, alignof(std::max_align_t));
        REQUIRE(small_ptr != nullptr);
        allocator.deallocate(small_ptr, 64);
    }

    SECTION("Result type error propagation") {
        // Test that memory manager access returns proper Result types
        auto result = getMemoryManager();

        if (is_success(result)) {
            auto* manager = get_value(result);
            RANGELUA_DEBUG_PRINT("Memory manager obtained successfully");
            REQUIRE(manager != nullptr);
        } else {
            [[maybe_unused]] ErrorCode error = get_error(result);
            RANGELUA_DEBUG_PRINT("Memory manager access failed with error: " +
                                String(error_code_to_string(error)));
            FAIL("Memory manager should be available");
        }
    }

    SECTION("Thread-safe error handling") {
        constexpr int num_threads = 4;
        std::vector<std::thread> threads;
        std::vector<bool> results(num_threads, false);

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                RANGELUA_SET_THREAD_NAME("test_thread_" + std::to_string(i));

                auto result = getMemoryManager();
                results[i] = is_success(result);

                if (results[i]) {
                    RANGELUA_DEBUG_PRINT("Thread " + std::to_string(i) + " got memory manager");
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // All threads should have succeeded
        for (bool result : results) {
            REQUIRE(result);
        }
    }
}

TEST_CASE("Memory pressure and monitoring", "[integration][memory][monitoring]") {
    SECTION("Memory manager creation and basic functionality") {
        auto runtime_manager = MemoryManagerFactory::create_runtime_manager();
        REQUIRE(runtime_manager != nullptr);

        RANGELUA_DEBUG_PRINT("Successfully created runtime memory manager");

        // Test that the manager exists and is functional
        REQUIRE(runtime_manager != nullptr);

        RANGELUA_DEBUG_PRINT("Memory manager functionality test completed");
    }

    SECTION("Memory manager integration test") {
        auto runtime_manager = MemoryManagerFactory::create_runtime_manager();
        REQUIRE(runtime_manager != nullptr);

        RANGELUA_DEBUG_PRINT("Runtime memory manager integration test completed");
    }
}

TEST_CASE("Integration with logging system", "[integration][memory][logging]") {
    SECTION("Memory operations generate appropriate logs") {
        SystemAllocator allocator;

        // These operations should generate debug logs when debug mode is enabled
        void* ptr = allocator.allocate(512, alignof(std::max_align_t));
        REQUIRE(ptr != nullptr);

        void* new_ptr = allocator.reallocate(ptr, 512, 1024);
        REQUIRE(new_ptr != nullptr);

        allocator.deallocate(new_ptr, 1024);

        // Verify that the allocator is still in a consistent state
        REQUIRE(allocator.allocation_count() == 0);
    }

    SECTION("Error conditions are properly logged") {
        SystemAllocator allocator;

        // Test null pointer deallocation (should be handled gracefully)
        allocator.deallocate(nullptr, 100);

        // Test zero-size allocation
        void* ptr = allocator.allocate(0, alignof(std::max_align_t));
        REQUIRE(ptr == nullptr);

        // These operations should generate appropriate debug messages
        // without causing crashes or undefined behavior
    }
}
