#pragma once

/**
 * @file memory.hpp
 * @brief Memory management and garbage collection
 * @version 0.1.0
 */

#include <memory>
#include <unordered_set>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/types.hpp"

namespace rangelua::runtime {

    /**
     * @brief Memory manager interface
     */
    class MemoryManager {
    public:
        virtual ~MemoryManager() = default;

        virtual void* allocate(Size size) = 0;
        virtual void deallocate(void* ptr, Size size) = 0;
        virtual void* reallocate(void* ptr, Size old_size, Size new_size) = 0;

        [[nodiscard]] virtual Size total_allocated() const noexcept = 0;
        [[nodiscard]] virtual Size allocation_count() const noexcept = 0;
    };

    /**
     * @brief Garbage collector interface
     */
    class GarbageCollector {
    public:
        virtual ~GarbageCollector() = default;

        virtual void collect() = 0;
        virtual void mark_phase() = 0;
        virtual void sweep_phase() = 0;
        virtual void add_root(void* ptr) = 0;
        virtual void remove_root(void* ptr) = 0;
    };

}  // namespace rangelua::runtime
