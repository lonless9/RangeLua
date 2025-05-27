/**
 * @file test_memory.cpp
 * @brief Comprehensive unit tests for memory management system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/runtime/memory.hpp>
#include <rangelua/core/error.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

#include <thread>
#include <vector>
#include <chrono>

using namespace rangelua;
using namespace rangelua::runtime;

TEST_CASE("SystemAllocator basic functionality", "[memory][allocator]") {
    SystemAllocator allocator;

    SECTION("Basic allocation and deallocation") {
        constexpr Size test_size = 1024;
        void* ptr = allocator.allocate(test_size, alignof(std::max_align_t));

        REQUIRE(ptr != nullptr);
        REQUIRE(allocator.total_allocated() >= test_size);
        REQUIRE(allocator.allocation_count() == 1);

        allocator.deallocate(ptr, test_size);
        REQUIRE(allocator.allocation_count() == 0);
    }

    SECTION("Zero size allocation") {
        void* ptr = allocator.allocate(0, alignof(std::max_align_t));
        REQUIRE(ptr == nullptr);
        REQUIRE(allocator.allocation_count() == 0);
    }

    SECTION("Aligned allocation") {
        constexpr Size test_size = 512;
        constexpr Size alignment = 64;

        void* ptr = allocator.allocate(test_size, alignment);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);

        allocator.deallocate(ptr, test_size);
    }

    SECTION("Reallocation") {
        constexpr Size initial_size = 256;
        constexpr Size new_size = 512;

        void* ptr = allocator.allocate(initial_size, alignof(std::max_align_t));
        REQUIRE(ptr != nullptr);

        void* new_ptr = allocator.reallocate(ptr, initial_size, new_size);
        REQUIRE(new_ptr != nullptr);

        allocator.deallocate(new_ptr, new_size);
    }

    SECTION("Null pointer handling") {
        // Should not crash
        allocator.deallocate(nullptr, 100);

        // Should allocate new memory
        void* ptr = allocator.reallocate(nullptr, 0, 256);
        REQUIRE(ptr != nullptr);
        allocator.deallocate(ptr, 256);

        // Should deallocate
        ptr = allocator.allocate(128, alignof(std::max_align_t));
        void* result = allocator.reallocate(ptr, 128, 0);
        REQUIRE(result == nullptr);
    }
}

TEST_CASE("PoolAllocator functionality", "[memory][pool]") {
    using TestPool = PoolAllocator<64, 16>;
    TestPool pool;

    SECTION("Pool allocation within block size") {
        void* ptr = pool.allocate(32, alignof(std::max_align_t));
        REQUIRE(ptr != nullptr);
        REQUIRE(pool.allocation_count() == 1);

        pool.deallocate(ptr, 32);
        REQUIRE(pool.allocation_count() == 0);
    }

    SECTION("Pool allocation exceeding block size") {
        void* ptr = pool.allocate(128, alignof(std::max_align_t));  // Larger than block size (64)
        REQUIRE(ptr == nullptr);
        REQUIRE(pool.allocation_count() == 0);
    }

    SECTION("Pool exhaustion") {
        std::vector<void*> ptrs;

        // Allocate all blocks
        for (Size i = 0; i < 16; ++i) {
            void* ptr = pool.allocate(64, alignof(std::max_align_t));
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }

        // Pool should be exhausted
        void* ptr = pool.allocate(64, alignof(std::max_align_t));
        REQUIRE(ptr == nullptr);

        // Clean up
        for (void* p : ptrs) {
            pool.deallocate(p, 64);
        }
    }
}

TEST_CASE("ObjectPool functionality", "[memory][object_pool]") {
    constexpr Size object_size = 32;
    constexpr Size pool_size = 8;
    ObjectPool pool(object_size, pool_size);

    SECTION("Basic object allocation") {
        void* ptr = pool.allocate();
        REQUIRE(ptr != nullptr);
        REQUIRE(pool.owns(ptr));
        REQUIRE(pool.availableObjects() == pool_size - 1);

        pool.deallocate(ptr);
        REQUIRE(pool.availableObjects() == pool_size);
    }

    SECTION("Pool ownership verification") {
        void* ptr = pool.allocate();
        REQUIRE(pool.owns(ptr));

        // External pointer should not be owned
        int external_var = 42;
        REQUIRE_FALSE(pool.owns(&external_var));

        pool.deallocate(ptr);
    }

    SECTION("Null pointer handling") {
        // Should not crash
        pool.deallocate(nullptr);
        REQUIRE(pool.availableObjects() == pool_size);
    }
}

TEST_CASE("MemoryManagerFactory", "[memory][factory]") {
    SECTION("System manager creation") {
        auto manager = MemoryManagerFactory::create_system_manager();
        REQUIRE(manager != nullptr);

        auto obj = manager->make_unique<int>(42);
        REQUIRE(obj != nullptr);
        REQUIRE(*obj == 42);
    }

    SECTION("Pool manager creation") {
        auto manager = MemoryManagerFactory::create_pool_manager<64, 128>();
        REQUIRE(manager != nullptr);
    }

    SECTION("Runtime manager creation") {
        auto runtime_manager = MemoryManagerFactory::create_runtime_manager();
        REQUIRE(runtime_manager != nullptr);
    }
}

TEST_CASE("Thread-safe memory management", "[memory][threading]") {
    SECTION("Concurrent allocations") {
        SystemAllocator allocator;
        constexpr int num_threads = 4;
        constexpr int allocations_per_thread = 100;

        std::vector<std::thread> threads;
        std::vector<std::vector<void*>> thread_ptrs(num_threads);

        // Launch threads
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < allocations_per_thread; ++i) {
                    void* ptr = allocator.allocate(64, alignof(std::max_align_t));
                    if (ptr) {
                        thread_ptrs[t].push_back(ptr);
                    }
                }
            });
        }

        // Wait for completion
        for (auto& thread : threads) {
            thread.join();
        }

        // Verify allocations
        Size total_allocations = 0;
        for (const auto& ptrs : thread_ptrs) {
            total_allocations += ptrs.size();
        }

        REQUIRE(total_allocations <= num_threads * allocations_per_thread);
        REQUIRE(allocator.allocation_count() == total_allocations);

        // Clean up
        for (int t = 0; t < num_threads; ++t) {
            for (void* ptr : thread_ptrs[t]) {
                allocator.deallocate(ptr, 64);
            }
        }

        REQUIRE(allocator.allocation_count() == 0);
    }
}

TEST_CASE("Memory manager access functions", "[memory][access]") {
    SECTION("getMemoryManager returns valid manager") {
        auto result = getMemoryManager();
        REQUIRE(is_success(result));

        auto* manager = get_value(result);
        REQUIRE(manager != nullptr);
    }

    SECTION("getGarbageCollector returns valid collector") {
        auto result = getGarbageCollector();
        REQUIRE(is_success(result));

        auto* gc = get_value(result);
        REQUIRE(gc != nullptr);
    }

    SECTION("Thread-local consistency") {
        auto result1 = getMemoryManager();
        auto result2 = getMemoryManager();

        REQUIRE(is_success(result1));
        REQUIRE(is_success(result2));
        REQUIRE(get_value(result1) == get_value(result2));
    }
}

TEST_CASE("Error handling in memory operations", "[memory][error]") {
    SECTION("Allocation failure handling") {
        SystemAllocator allocator;

        // Test with a more reasonable large size that won't trigger AddressSanitizer
        // Use 1GB which should be large enough to potentially fail on some systems
        // but not so large as to trigger sanitizer errors
        constexpr Size large_size = Size{1024} * 1024 * 1024;  // 1GB

        void* ptr = allocator.allocate(large_size, alignof(std::max_align_t));
        // This may or may not fail depending on available memory
        // We just check that the allocator handles it gracefully

        if (ptr) {
            // If allocation succeeded, clean it up
            allocator.deallocate(ptr, large_size);
        }

        // The allocator should still be functional regardless
        void* small_ptr = allocator.allocate(64, alignof(std::max_align_t));
        REQUIRE(small_ptr != nullptr);
        allocator.deallocate(small_ptr, 64);
    }
}
