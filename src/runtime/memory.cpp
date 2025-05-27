/**
 * @file memory.cpp
 * @brief Modern C++20 memory management implementation
 * @version 0.1.0
 */

#include <rangelua/runtime/memory.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

namespace rangelua::runtime {

    // SystemAllocator implementation
    void* SystemAllocator::allocate(Size size, Size alignment) noexcept {
        if (size == 0) {
            return nullptr;
        }

        void* ptr = nullptr;

        // Use aligned allocation if available
        if (alignment > alignof(std::max_align_t)) {
            ptr = std::aligned_alloc(alignment, size);
        } else {
            ptr = std::malloc(size);
        }

        if (ptr) {
            total_allocated_.fetch_add(size, std::memory_order_relaxed);
            allocation_count_.fetch_add(1, std::memory_order_relaxed);
        }

        return ptr;
    }

    void SystemAllocator::deallocate(void* ptr, Size size) noexcept {
        if (ptr) {
            std::free(ptr);
            total_allocated_.fetch_sub(size, std::memory_order_relaxed);
            allocation_count_.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    void* SystemAllocator::reallocate(void* ptr, Size old_size, Size new_size) noexcept {
        if (new_size == 0) {
            deallocate(ptr, old_size);
            return nullptr;
        }

        if (!ptr) {
            return allocate(new_size, alignof(std::max_align_t));
        }

        void* new_ptr = std::realloc(ptr, new_size);
        if (new_ptr) {
            // Update statistics
            if (new_size > old_size) {
                total_allocated_.fetch_add(new_size - old_size, std::memory_order_relaxed);
            } else if (old_size > new_size) {
                total_allocated_.fetch_sub(old_size - new_size, std::memory_order_relaxed);
            }
        }

        return new_ptr;
    }

    // PoolAllocator implementation
    template <Size BlockSize, Size BlockCount>
    void PoolAllocator<BlockSize, BlockCount>::initialize_pool() {
        constexpr Size pool_size = BlockSize * BlockCount;
        pool_memory_ = std::aligned_alloc(alignof(std::max_align_t), pool_size);

        if (!pool_memory_) {
            throw std::bad_alloc();
        }

        // Initialize free list
        char* current = static_cast<char*>(pool_memory_);
        free_list_ = current;

        for (Size i = 0; i < BlockCount - 1; ++i) {
            void** next_ptr = reinterpret_cast<void**>(current);
            current += BlockSize;
            *next_ptr = current;
        }

        // Last block points to nullptr
        void** last_ptr = reinterpret_cast<void**>(current);
        *last_ptr = nullptr;
    }

    template <Size BlockSize, Size BlockCount>
    void PoolAllocator<BlockSize, BlockCount>::cleanup_pool() {
        if (pool_memory_) {
            std::free(pool_memory_);
            pool_memory_ = nullptr;
            free_list_ = nullptr;
        }
    }

    template <Size BlockSize, Size BlockCount>
    void* PoolAllocator<BlockSize, BlockCount>::allocate(Size size, Size /* alignment */) noexcept {
        if (size > BlockSize) {
            return nullptr;  // Size too large for this pool
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!free_list_) {
            return nullptr;  // Pool exhausted
        }

        void* result = free_list_;
        free_list_ = *static_cast<void**>(free_list_);
        ++allocated_blocks_;

        return result;
    }

    template <Size BlockSize, Size BlockCount>
    void PoolAllocator<BlockSize, BlockCount>::deallocate(void* ptr, Size /* size */) noexcept {
        if (!ptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Verify pointer is within pool bounds
        char* pool_start = static_cast<char*>(pool_memory_);
        char* pool_end = pool_start + (BlockSize * BlockCount);
        char* ptr_char = static_cast<char*>(ptr);

        if (ptr_char < pool_start || ptr_char >= pool_end) {
            return;  // Invalid pointer
        }

        // Add to free list
        *static_cast<void**>(ptr) = free_list_;
        free_list_ = ptr;
        --allocated_blocks_;
    }

    template <Size BlockSize, Size BlockCount>
    void* PoolAllocator<BlockSize, BlockCount>::reallocate(void* ptr, Size old_size, Size new_size) noexcept {
        if (new_size > BlockSize) {
            return nullptr;  // Size too large for this pool
        }

        if (!ptr) {
            return allocate(new_size, alignof(std::max_align_t));
        }

        if (new_size == 0) {
            deallocate(ptr, old_size);
            return nullptr;
        }

        // For pool allocator, we can't resize in place
        void* new_ptr = allocate(new_size, alignof(std::max_align_t));
        if (new_ptr) {
            Size copy_size = std::min(old_size, new_size);
            std::memcpy(new_ptr, ptr, copy_size);
            deallocate(ptr, old_size);
        }

        return new_ptr;
    }

    // ObjectPool implementation
    ObjectPool::ObjectPool(Size objectSize, Size poolSize)
        : objectSize_(objectSize), poolSize_(poolSize), pool_(nullptr) {
        expandPool();
    }

    ObjectPool::~ObjectPool() {
        if (pool_) {
            std::free(pool_);
        }
    }

    void* ObjectPool::allocate() {
        if (freeList_.empty()) {
            expandPool();
        }

        if (freeList_.empty()) {
            return nullptr;  // Pool exhausted
        }

        void* ptr = freeList_.back();
        freeList_.pop_back();
        return ptr;
    }

    void ObjectPool::deallocate(void* ptr) {
        if (ptr && owns(ptr)) {
            freeList_.push_back(ptr);
        }
    }

    Size ObjectPool::availableObjects() const noexcept {
        return freeList_.size();
    }

    bool ObjectPool::owns(void* ptr) const noexcept {
        if (!pool_ || !ptr) {
            return false;
        }

        char* pool_start = static_cast<char*>(pool_);
        char* pool_end = pool_start + (objectSize_ * poolSize_);
        char* ptr_char = static_cast<char*>(ptr);

        return ptr_char >= pool_start && ptr_char < pool_end;
    }

    void ObjectPool::expandPool() {
        Size pool_size = objectSize_ * poolSize_;
        pool_ = std::realloc(pool_, pool_size);

        if (!pool_) {
            throw std::bad_alloc();
        }

        // Add new objects to free list
        char* current = static_cast<char*>(pool_);
        for (Size i = 0; i < poolSize_; ++i) {
            freeList_.push_back(current);
            current += objectSize_;
        }
    }

    // Global runtime memory management
    static RuntimeMemoryManager* g_memory_manager = nullptr;
    static GarbageCollector* g_garbage_collector = nullptr;

    RuntimeMemoryManager& getMemoryManager() {
        if (!g_memory_manager) {
            // Create default memory manager
            static auto default_manager = MemoryManagerFactory::create_runtime_manager();
            g_memory_manager = default_manager.get();
        }
        return *g_memory_manager;
    }

    void setMemoryManager(RuntimeMemoryManager* manager) {
        g_memory_manager = manager;
    }

    GarbageCollector& getGarbageCollector() {
        if (!g_garbage_collector) {
            throw std::runtime_error("No garbage collector has been set");
        }
        return *g_garbage_collector;
    }

    void setGarbageCollector(GarbageCollector* gc) {
        g_garbage_collector = gc;
    }

    // MemoryManagerScope implementation
    MemoryManagerScope::MemoryManagerScope(RuntimeMemoryManager* manager)
        : previousManager_(&getMemoryManager()) {
        setMemoryManager(manager);
    }

    MemoryManagerScope::~MemoryManagerScope() {
        setMemoryManager(previousManager_);
    }

    // Explicit template instantiations for common sizes
    template class PoolAllocator<32, 1024>;    // Small objects
    template class PoolAllocator<64, 512>;     // Medium objects
    template class PoolAllocator<64, 1024>;    // Medium objects (larger pool)
    template class PoolAllocator<128, 256>;    // Large objects
    template class PoolAllocator<256, 128>;    // Very large objects

}  // namespace rangelua::runtime
