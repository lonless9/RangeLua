#pragma once

/**
 * @file memory.hpp
 * @brief Memory management and garbage collection interfaces
 * @version 0.1.0
 *
 * This file provides the core interfaces for memory management and garbage collection
 * in RangeLua. It's designed to work with the hybrid GC strategy and support
 * future migration to different GC algorithms.
 */

#include <chrono>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/types.hpp"

namespace rangelua::runtime {

    // Forward declarations
    class GCObject;

    /**
     * @brief Memory allocation statistics
     */
    struct MemoryStats {
        Size totalAllocated = 0;
        Size totalFreed = 0;
        Size currentAllocated = 0;
        Size peakAllocated = 0;
        Size allocationCount = 0;
        Size deallocationCount = 0;
        Size reallocCount = 0;
    };

    /**
     * @brief Memory manager interface with enhanced features
     *
     * Provides memory allocation, deallocation, and tracking capabilities
     * optimized for garbage-collected environments.
     */
    class MemoryManager {
    public:
        virtual ~MemoryManager() = default;

        // Basic allocation interface
        virtual void* allocate(Size size) = 0;
        virtual void deallocate(void* ptr, Size size) = 0;
        virtual void* reallocate(void* ptr, Size old_size, Size new_size) = 0;

        // Aligned allocation for GC objects
        virtual void* allocateAligned(Size size, Size alignment) = 0;
        virtual void deallocateAligned(void* ptr, Size size, Size alignment) = 0;

        // Statistics and monitoring
        [[nodiscard]] virtual const MemoryStats& stats() const noexcept = 0;
        virtual void resetStats() noexcept = 0;

        // Memory pressure detection
        [[nodiscard]] virtual bool isMemoryPressure() const noexcept = 0;
        virtual void setMemoryPressureThreshold(Size threshold) noexcept = 0;

        // GC integration
        virtual void notifyGCStart() noexcept = 0;
        virtual void notifyGCEnd(Size freedBytes) noexcept = 0;
    };

    /**
     * @brief Enhanced garbage collector interface
     *
     * Supports multiple GC strategies and provides comprehensive
     * monitoring and control capabilities.
     */
    class GarbageCollector {
    public:
        virtual ~GarbageCollector() = default;

        // Core GC interface
        virtual void collect() = 0;
        virtual void mark_phase() = 0;
        virtual void sweep_phase() = 0;

        // Root management
        virtual void add_root(void* ptr) = 0;
        virtual void remove_root(void* ptr) = 0;
        virtual void add_root(GCObject* obj) = 0;
        virtual void remove_root(GCObject* obj) = 0;

        // Advanced features
        virtual void setMemoryManager(MemoryManager* manager) noexcept = 0;
        virtual void requestCollection() noexcept = 0;
        virtual void emergencyCollection() = 0;

        // Configuration
        virtual void setCollectionThreshold(Size threshold) noexcept = 0;
        virtual void setCollectionInterval(std::chrono::milliseconds interval) noexcept = 0;

        // Monitoring
        [[nodiscard]] virtual bool isCollecting() const noexcept = 0;
        [[nodiscard]] virtual Size objectCount() const noexcept = 0;
        [[nodiscard]] virtual Size memoryUsage() const noexcept = 0;
    };

    /**
     * @brief Default memory manager implementation
     *
     * Provides a basic memory manager with statistics tracking
     * and memory pressure detection.
     */
    class DefaultMemoryManager : public MemoryManager {
    public:
        DefaultMemoryManager() = default;
        ~DefaultMemoryManager() override = default;

        // MemoryManager interface
        void* allocate(Size size) override;
        void deallocate(void* ptr, Size size) override;
        void* reallocate(void* ptr, Size old_size, Size new_size) override;
        void* allocateAligned(Size size, Size alignment) override;
        void deallocateAligned(void* ptr, Size size, Size alignment) override;

        [[nodiscard]] const MemoryStats& stats() const noexcept override { return stats_; }
        void resetStats() noexcept override { stats_ = {}; }

        [[nodiscard]] bool isMemoryPressure() const noexcept override;
        void setMemoryPressureThreshold(Size threshold) noexcept override {
            memoryPressureThreshold_ = threshold;
        }

        void notifyGCStart() noexcept override { gcActive_ = true; }
        void notifyGCEnd(Size freedBytes) noexcept override;

    private:
        MemoryStats stats_;
        Size memoryPressureThreshold_ = 64 * 1024 * 1024;  // 64MB
        bool gcActive_ = false;

        void updateStats(Size size, bool allocation);
    };

    /**
     * @brief Memory pool for small object allocation
     *
     * Optimized allocator for small GC objects to reduce
     * fragmentation and improve cache locality.
     */
    class ObjectPool {
    public:
        explicit ObjectPool(Size objectSize, Size poolSize = 1024);
        ~ObjectPool();

        void* allocate();
        void deallocate(void* ptr);

        [[nodiscard]] Size objectSize() const noexcept { return objectSize_; }
        [[nodiscard]] Size availableObjects() const noexcept;
        [[nodiscard]] bool owns(void* ptr) const noexcept;

    private:
        Size objectSize_;
        Size poolSize_;
        void* pool_;
        std::vector<void*> freeList_;

        void expandPool();
    };

    /**
     * @brief RAII memory manager selector
     *
     * Allows temporary switching of memory managers with
     * automatic restoration.
     */
    class MemoryManagerScope {
    public:
        explicit MemoryManagerScope(MemoryManager* manager);
        ~MemoryManagerScope();

        // Non-copyable, non-movable
        MemoryManagerScope(const MemoryManagerScope&) = delete;
        MemoryManagerScope& operator=(const MemoryManagerScope&) = delete;
        MemoryManagerScope(MemoryManagerScope&&) = delete;
        MemoryManagerScope& operator=(MemoryManagerScope&&) = delete;

    private:
        MemoryManager* previousManager_;
    };

    /**
     * @brief Global memory manager access
     */
    MemoryManager& getMemoryManager();
    void setMemoryManager(MemoryManager* manager);

    /**
     * @brief Global garbage collector access
     */
    GarbageCollector& getGarbageCollector();
    void setGarbageCollector(GarbageCollector* gc);

}  // namespace rangelua::runtime
