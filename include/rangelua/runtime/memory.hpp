#pragma once

/**
 * @file memory.hpp
 * @brief Unified memory management system with RAII, thread safety, and garbage collection
 * @version 0.1.0
 *
 * This file provides a comprehensive memory management system for RangeLua that includes:
 * - Modern C++20 memory allocators with RAII guarantees
 * - Runtime memory management with garbage collection support
 * - Thread-safe memory tracking and statistics
 * - Pool allocators for performance optimization
 * - Integration with Lua's memory management patterns
 */

#include <rangelua/core/concepts.hpp>
#include <rangelua/core/error.hpp>
#include <rangelua/core/types.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

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
     * @brief Thread-safe memory allocator with RAII guarantees
     */
    class MemoryAllocator {
    public:
        virtual ~MemoryAllocator() = default;

        // Non-copyable, non-movable by default (derived classes can override)
        MemoryAllocator(const MemoryAllocator&) = delete;
        MemoryAllocator& operator=(const MemoryAllocator&) = delete;
        MemoryAllocator(MemoryAllocator&&) = delete;
        MemoryAllocator& operator=(MemoryAllocator&&) = delete;

        /**
         * @brief Allocate memory block
         * @param size Size in bytes
         * @param alignment Memory alignment requirement
         * @return Pointer to allocated memory or nullptr on failure
         */
        [[nodiscard]] virtual void* allocate(Size size, Size alignment) noexcept = 0;

        /**
         * @brief Allocate memory block with default alignment
         * @param size Size in bytes
         * @return Pointer to allocated memory or nullptr on failure
         */
        [[nodiscard]] void* allocate(Size size) noexcept {
            return allocate(size, alignof(std::max_align_t));
        }

        /**
         * @brief Deallocate memory block
         * @param ptr Pointer to memory block
         * @param size Size of the block
         */
        virtual void deallocate(void* ptr, Size size) noexcept = 0;

        /**
         * @brief Reallocate memory block
         * @param ptr Existing pointer (can be nullptr)
         * @param old_size Old size
         * @param new_size New size
         * @return Pointer to reallocated memory or nullptr on failure
         */
        [[nodiscard]] virtual void*
        reallocate(void* ptr, Size old_size, Size new_size) noexcept = 0;

        /**
         * @brief Get total allocated bytes
         */
        [[nodiscard]] virtual Size total_allocated() const noexcept = 0;

        /**
         * @brief Get allocation count
         */
        [[nodiscard]] virtual Size allocation_count() const noexcept = 0;

    protected:
        MemoryAllocator() = default;
    };

    /**
     * @brief Default system allocator with tracking
     */
    class SystemAllocator : public MemoryAllocator {
    public:
        SystemAllocator() = default;
        ~SystemAllocator() override = default;

        // Non-copyable, non-movable (due to atomic members)
        SystemAllocator(const SystemAllocator&) = delete;
        SystemAllocator& operator=(const SystemAllocator&) = delete;
        SystemAllocator(SystemAllocator&&) = delete;
        SystemAllocator& operator=(SystemAllocator&&) = delete;

        [[nodiscard]] void* allocate(Size size, Size alignment) noexcept override;
        void deallocate(void* ptr, Size size) noexcept override;
        [[nodiscard]] void* reallocate(void* ptr, Size old_size, Size new_size) noexcept override;

        [[nodiscard]] Size total_allocated() const noexcept override {
            return total_allocated_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] Size allocation_count() const noexcept override {
            return allocation_count_.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<Size> total_allocated_{0};
        std::atomic<Size> allocation_count_{0};
    };

    /**
     * @brief Pool allocator for fixed-size objects
     */
    template <Size BlockSize, Size BlockCount = 1024>
    class PoolAllocator : public MemoryAllocator {
    public:
        PoolAllocator() { initialize_pool(); }

        ~PoolAllocator() override { cleanup_pool(); }

        // Non-copyable, non-movable (due to mutex and state)
        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) = delete;
        PoolAllocator& operator=(PoolAllocator&&) = delete;

        [[nodiscard]] void* allocate(Size size, Size alignment) noexcept override;
        void deallocate(void* ptr, Size size) noexcept override;
        [[nodiscard]] void* reallocate(void* ptr, Size old_size, Size new_size) noexcept override;

        [[nodiscard]] Size total_allocated() const noexcept override {
            std::lock_guard<std::mutex> lock(mutex_);
            return allocated_blocks_ * BlockSize;
        }

        [[nodiscard]] Size allocation_count() const noexcept override {
            std::lock_guard<std::mutex> lock(mutex_);
            return allocated_blocks_;
        }

    private:
        void initialize_pool();
        void cleanup_pool();

        mutable std::mutex mutex_;
        void* pool_memory_ = nullptr;
        void* free_list_ = nullptr;
        Size allocated_blocks_ = 0;
    };

    /**
     * @brief RAII memory resource wrapper
     */
    template <typename T>
    class ManagedResource {
    public:
        using resource_type = T;

        explicit ManagedResource(T resource) noexcept : resource_(std::move(resource)) {}

        ~ManagedResource() { release(); }

        // Non-copyable, movable
        ManagedResource(const ManagedResource&) = delete;
        ManagedResource& operator=(const ManagedResource&) = delete;
        ManagedResource(ManagedResource&& other) noexcept : resource_(std::move(other.resource_)) {
            other.released_ = true;
        }
        ManagedResource& operator=(ManagedResource&& other) noexcept {
            if (this != &other) {
                release();
                resource_ = std::move(other.resource_);
                released_ = false;
                other.released_ = true;
            }
            return *this;
        }

        [[nodiscard]] const T& get() const noexcept { return resource_; }
        [[nodiscard]] T& get() noexcept { return resource_; }

        void release() noexcept {
            if (!released_) {
                if constexpr (requires { resource_.release(); }) {
                    resource_.release();
                }
                released_ = true;
            }
        }

        [[nodiscard]] bool is_released() const noexcept { return released_; }

    private:
        T resource_;
        bool released_ = false;
    };

    /**
     * @brief Runtime memory manager interface with enhanced features
     *
     * Provides memory allocation, deallocation, and tracking capabilities
     * optimized for garbage-collected environments.
     */
    class RuntimeMemoryManager {
    public:
        virtual ~RuntimeMemoryManager() = default;

        // Non-copyable, non-movable by default
        RuntimeMemoryManager(const RuntimeMemoryManager&) = delete;
        RuntimeMemoryManager& operator=(const RuntimeMemoryManager&) = delete;
        RuntimeMemoryManager(RuntimeMemoryManager&&) = delete;
        RuntimeMemoryManager& operator=(RuntimeMemoryManager&&) = delete;

        // Public allocation interface for VM and other runtime components
        [[nodiscard]] void* allocate(Size size) { return do_allocate(size); }

        void deallocate(void* ptr, Size size) { do_deallocate(ptr, size); }

        [[nodiscard]] void* reallocate(void* ptr, Size old_size, Size new_size) {
            return do_reallocate(ptr, old_size, new_size);
        }

        [[nodiscard]] void* allocateAligned(Size size, Size alignment) {
            return do_allocateAligned(size, alignment);
        }

        void deallocateAligned(void* ptr, Size size, Size alignment) {
            do_deallocateAligned(ptr, size, alignment);
        }

        // Statistics and monitoring
        [[nodiscard]] virtual const MemoryStats& stats() const noexcept = 0;
        virtual void resetStats() noexcept = 0;

        // Memory pressure detection
        [[nodiscard]] virtual bool isMemoryPressure() const noexcept = 0;
        virtual void setMemoryPressureThreshold(Size threshold) noexcept = 0;

        // GC integration
        virtual void notifyGCStart() noexcept = 0;
        virtual void notifyGCEnd(Size freedBytes) noexcept = 0;

    protected:
        RuntimeMemoryManager() = default;

        // Implementation interface for derived classes
        virtual void* do_allocate(Size size) = 0;
        virtual void do_deallocate(void* ptr, Size size) = 0;
        virtual void* do_reallocate(void* ptr, Size old_size, Size new_size) = 0;

        // Aligned allocation for GC objects
        virtual void* do_allocateAligned(Size size, Size alignment) = 0;
        virtual void do_deallocateAligned(void* ptr, Size size, Size alignment) = 0;
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

        // Non-copyable, non-movable by default
        GarbageCollector(const GarbageCollector&) = delete;
        GarbageCollector& operator=(const GarbageCollector&) = delete;
        GarbageCollector(GarbageCollector&&) = delete;
        GarbageCollector& operator=(GarbageCollector&&) = delete;

    public:
        // Root management (public for GCRoot RAII class)
        virtual void add_root(void* ptr) = 0;
        virtual void remove_root(void* ptr) = 0;
        virtual void add_root(GCObject* obj) = 0;
        virtual void remove_root(GCObject* obj) = 0;

    protected:
        GarbageCollector() = default;

        // Core GC interface
        virtual void collect() = 0;
        virtual void mark_phase() = 0;
        virtual void sweep_phase() = 0;

        // Advanced features
        virtual void setMemoryManager(RuntimeMemoryManager* manager) noexcept = 0;
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
     * @brief Default runtime memory manager implementation
     *
     * Provides a basic memory manager with statistics tracking
     * and memory pressure detection.
     */
    class DefaultRuntimeMemoryManager : public RuntimeMemoryManager {
    public:
        explicit DefaultRuntimeMemoryManager(UniquePtr<MemoryAllocator> allocator)
            : allocator_(std::move(allocator)) {}
        ~DefaultRuntimeMemoryManager() override = default;

        // Non-copyable, non-movable
        DefaultRuntimeMemoryManager(const DefaultRuntimeMemoryManager&) = delete;
        DefaultRuntimeMemoryManager& operator=(const DefaultRuntimeMemoryManager&) = delete;
        DefaultRuntimeMemoryManager(DefaultRuntimeMemoryManager&&) = delete;
        DefaultRuntimeMemoryManager& operator=(DefaultRuntimeMemoryManager&&) = delete;

        // RuntimeMemoryManager implementation interface
        void* do_allocate(Size size) override {
            void* ptr = allocator_->allocate(size);
            if (ptr) {
                updateStats(size, true);
            }
            return ptr;
        }

        void do_deallocate(void* ptr, Size size) override {
            if (ptr) {
                allocator_->deallocate(ptr, size);
                updateStats(size, false);
            }
        }

        void* do_reallocate(void* ptr, Size old_size, Size new_size) override {
            void* new_ptr = allocator_->reallocate(ptr, old_size, new_size);
            if (new_ptr && new_size != old_size) {
                updateStats(old_size, false);
                updateStats(new_size, true);
                stats_.reallocCount++;
            }
            return new_ptr;
        }

        void* do_allocateAligned(Size size, Size alignment) override {
            void* ptr = allocator_->allocate(size, alignment);
            if (ptr) {
                updateStats(size, true);
            }
            return ptr;
        }

        void do_deallocateAligned(void* ptr, Size size, Size /* alignment */) override {
            if (ptr) {
                allocator_->deallocate(ptr, size);
                updateStats(size, false);
            }
        }

        [[nodiscard]] const MemoryStats& stats() const noexcept override { return stats_; }
        void resetStats() noexcept override { stats_ = {}; }

        [[nodiscard]] bool isMemoryPressure() const noexcept override {
            return stats_.currentAllocated > memoryPressureThreshold_;
        }

        void setMemoryPressureThreshold(Size threshold) noexcept override {
            memoryPressureThreshold_ = threshold;
        }

        void notifyGCStart() noexcept override { gcActive_ = true; }
        void notifyGCEnd(Size freedBytes) noexcept override {
            gcActive_ = false;
            stats_.totalFreed += freedBytes;
            if (stats_.currentAllocated >= freedBytes) {
                stats_.currentAllocated -= freedBytes;
            } else {
                stats_.currentAllocated = 0;
            }
        }

    private:
        UniquePtr<MemoryAllocator> allocator_;
        MemoryStats stats_;
        Size memoryPressureThreshold_ = Size{64} * 1024 * 1024;  // 64MB
        bool gcActive_ = false;

        void updateStats(Size size, bool allocation) {
            if (allocation) {
                stats_.totalAllocated += size;
                stats_.currentAllocated += size;
                stats_.allocationCount++;
                if (stats_.currentAllocated > stats_.peakAllocated) {
                    stats_.peakAllocated = stats_.currentAllocated;
                }
            } else {
                stats_.totalFreed += size;
                stats_.deallocationCount++;
                if (stats_.currentAllocated >= size) {
                    stats_.currentAllocated -= size;
                } else {
                    stats_.currentAllocated = 0;
                }
            }
        }
    };

    /**
     * @brief Core memory manager with dependency injection (for non-GC allocations)
     */
    class MemoryManager {
    public:
        explicit MemoryManager(UniquePtr<MemoryAllocator> allocator)
            : allocator_(std::move(allocator)) {}

        ~MemoryManager() = default;

        // Non-copyable, movable
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) noexcept = default;
        MemoryManager& operator=(MemoryManager&&) noexcept = default;

        /**
         * @brief Allocate memory with standard allocator for simplicity
         */
        template <typename T, typename... Args>
        [[nodiscard]] UniquePtr<T> make_unique(Args&&... args) {
            // For now, use standard allocator to avoid complex deleter issues
            // In a full implementation, we'd use the custom allocator
            return std::make_unique<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Get allocator statistics
         */
        [[nodiscard]] Size total_allocated() const noexcept {
            return allocator_->total_allocated();
        }

        [[nodiscard]] Size allocation_count() const noexcept {
            return allocator_->allocation_count();
        }

    private:
        UniquePtr<MemoryAllocator> allocator_;
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

        // Non-copyable, non-movable
        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;
        ObjectPool(ObjectPool&&) = delete;
        ObjectPool& operator=(ObjectPool&&) = delete;

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

    // Note: MemoryManagerScope removed to eliminate global state dependency
    // Use dependency injection pattern instead for memory manager management

    /**
     * @brief Factory for creating memory managers
     */
    class MemoryManagerFactory {
    public:
        /**
         * @brief Create system memory manager
         */
        static UniquePtr<MemoryManager> create_system_manager() {
            auto allocator = std::make_unique<SystemAllocator>();
            return std::make_unique<MemoryManager>(std::move(allocator));
        }

        /**
         * @brief Create pool memory manager
         */
        template <Size BlockSize, Size BlockCount = 1024>
        static UniquePtr<MemoryManager> create_pool_manager() {
            auto allocator = std::make_unique<PoolAllocator<BlockSize, BlockCount>>();
            return std::make_unique<MemoryManager>(std::move(allocator));
        }

        /**
         * @brief Create runtime memory manager
         */
        static UniquePtr<RuntimeMemoryManager> create_runtime_manager() {
            auto allocator = std::make_unique<SystemAllocator>();
            return std::make_unique<DefaultRuntimeMemoryManager>(std::move(allocator));
        }

        /**
         * @brief Create runtime memory manager with pool allocator
         */
        template <Size BlockSize, Size BlockCount = 1024>
        static UniquePtr<RuntimeMemoryManager> create_runtime_pool_manager() {
            auto allocator = std::make_unique<PoolAllocator<BlockSize, BlockCount>>();
            return std::make_unique<DefaultRuntimeMemoryManager>(std::move(allocator));
        }
    };

    // Thread-safe access functions for runtime memory management (no global state)
    /**
     * @brief Thread-local runtime memory manager access
     */
    Result<RuntimeMemoryManager*> getMemoryManager();

    /**
     * @brief Thread-local garbage collector access
     */
    Result<GarbageCollector*> getGarbageCollector();

}  // namespace rangelua::runtime

// Concept verification
static_assert(rangelua::RAIIResource<rangelua::runtime::ManagedResource<int>>);
