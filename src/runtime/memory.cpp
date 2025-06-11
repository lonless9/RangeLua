/**
 * @file memory.cpp
 * @brief Modern C++20 memory management implementation with error handling and debug integration
 * @version 0.2.0
 */

#include <rangelua/core/error.hpp>
#include <rangelua/runtime/gc.hpp>
#include <rangelua/runtime/memory.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

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

        if (alignment > alignof(std::max_align_t)) {
            ptr = std::aligned_alloc(alignment, size);
        } else {
            ptr = std::malloc(size);
        }

        if (ptr) {
            total_allocated_.fetch_add(size, std::memory_order_relaxed);
            allocation_count_.fetch_add(1, std::memory_order_relaxed);
        } else {
            log_error(ErrorCode::MEMORY_ERROR,
                      "SystemAllocator failed to allocate " + std::to_string(size) + " bytes");
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
            if (new_size > old_size) {
                total_allocated_.fetch_add(new_size - old_size, std::memory_order_relaxed);
            } else if (old_size > new_size) {
                total_allocated_.fetch_sub(old_size - new_size, std::memory_order_relaxed);
            }
        } else {
            log_error(ErrorCode::MEMORY_ERROR,
                      "SystemAllocator failed to reallocate from " + std::to_string(old_size) +
                          " to " + std::to_string(new_size) + " bytes");
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

        char* current = static_cast<char*>(pool_memory_);
        free_list_ = current;

        for (Size i = 0; i < BlockCount - 1; ++i) {
            void** next_ptr = reinterpret_cast<void**>(current);
            current += BlockSize;
            *next_ptr = current;
        }

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
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!free_list_) {
            return nullptr;
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

        char* pool_start = static_cast<char*>(pool_memory_);
        char* pool_end = pool_start + (BlockSize * BlockCount);
        char* ptr_char = static_cast<char*>(ptr);

        if (ptr_char < pool_start || ptr_char >= pool_end) {
            return;
        }

        *static_cast<void**>(ptr) = free_list_;
        free_list_ = ptr;
        --allocated_blocks_;
    }

    template <Size BlockSize, Size BlockCount>
    void* PoolAllocator<BlockSize, BlockCount>::reallocate(void* ptr, Size old_size, Size new_size) noexcept {
        if (new_size > BlockSize) {
            return nullptr;
        }

        if (!ptr) {
            return allocate(new_size, alignof(std::max_align_t));
        }

        if (new_size == 0) {
            deallocate(ptr, old_size);
            return nullptr;
        }

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
            return nullptr;
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

        char* current = static_cast<char*>(pool_);
        for (Size i = 0; i < poolSize_; ++i) {
            freeList_.push_back(current);
            current += objectSize_;
        }
    }

    // Thread-safe runtime memory management without global state
    namespace {
        // Use a unique_ptr to manage the lifetime of the thread-local GC
        // This ensures that the GC is destroyed when the thread exits.
        auto get_thread_local_gc_ptr() -> std::unique_ptr<GarbageCollector>& {
            thread_local std::unique_ptr<GarbageCollector> default_gc;
            if (!default_gc) {
                default_gc = std::make_unique<DefaultGarbageCollector>();
            }
            return default_gc;
        }
    } // anonymous namespace

    Result<RuntimeMemoryManager*> getMemoryManager() {
        thread_local auto default_manager = MemoryManagerFactory::create_runtime_manager();
        if (!default_manager) {
            return ErrorCode::MEMORY_ERROR;
        }
        return default_manager.get();
    }

    Result<GarbageCollector*> getGarbageCollector() {
        auto& gc_ptr = get_thread_local_gc_ptr();
        if (!gc_ptr) {
            return ErrorCode::MEMORY_ERROR;
        }
        return gc_ptr.get();
    }

    void cleanupThreadLocalGC() {
        auto& gc_ptr = get_thread_local_gc_ptr();
        if (gc_ptr) {
            if (auto* advanced_gc = dynamic_cast<AdvancedGarbageCollector*>(gc_ptr.get())) {
                advanced_gc->emergencyCollection();
            }
            gc_ptr.reset(); // This will call the destructor and free memory
            GC_LOG_INFO("Thread-local garbage collector cleaned up.");
        }
    }

    // Explicit template instantiations for common sizes
    template class PoolAllocator<32, 1024>;
    template class PoolAllocator<64, 512>;
    template class PoolAllocator<64, 1024>;
    template class PoolAllocator<128, 256>;
    template class PoolAllocator<256, 128>;

    template class PoolAllocator<64, 8>;
    template class PoolAllocator<64, 16>;
    template class PoolAllocator<64, 128>;

}  // namespace rangelua::runtime
