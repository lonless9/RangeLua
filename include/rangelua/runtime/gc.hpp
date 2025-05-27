#pragma once

/**
 * @file gc.hpp
 * @brief Modern C++20 garbage collection system for RangeLua - Declarations Only
 * @version 0.1.0
 *
 * This file contains only declarations for the GC system.
 * Implementations are provided in separate source files.
 *
 * Design Goals:
 * - Lua 5.5 semantic compatibility
 * - Modern C++20 features (concepts, RAII, smart pointers)
 * - Performance optimization over std::shared_ptr
 * - Clear separation of concerns
 */

#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/types.hpp"
#include "memory.hpp"

namespace rangelua::runtime {

    // Forward declarations
    class GCObject;
    class GCManager;
    template <typename T>
    class GCPtr;
    template <typename T>
    class WeakGCPtr;

    /**
     * @brief GC object concepts for type safety
     */
    template <typename T>
    concept GCManaged = requires(T* obj) {
        { obj->gcHeader() } -> std::convertible_to<GCHeader&>;
        { obj->mark() } -> std::same_as<void>;
        { obj->unmark() } -> std::same_as<void>;
        { obj->isMarked() } -> std::same_as<bool>;
        { obj->traverse(std::declval<std::function<void(GCObject*)>&>()) } -> std::same_as<void>;
    };

    /**
     * @brief Base class for all garbage-collected objects
     *
     * Provides the interface required for both reference counting
     * and tracing garbage collection strategies.
     */
    class GCObject {
    public:
        explicit GCObject(LuaType type) noexcept : header_(type) {}
        virtual ~GCObject() = default;

        // Non-copyable, non-movable (GC objects have identity)
        GCObject(const GCObject&) = delete;
        GCObject& operator=(const GCObject&) = delete;
        GCObject(GCObject&&) = delete;
        GCObject& operator=(GCObject&&) = delete;

        // GC interface (inline implementations for concept satisfaction)
        [[nodiscard]] GCHeader& gcHeader() noexcept { return header_; }
        [[nodiscard]] const GCHeader& gcHeader() const noexcept { return header_; }

        [[nodiscard]] LuaType type() const noexcept { return header_.type; }
        [[nodiscard]] bool isMarked() const noexcept { return header_.marked != 0; }

        void mark() noexcept { header_.marked = 1; }
        void unmark() noexcept { header_.marked = 0; }

        // Reference counting for hybrid approach
        void addRef() noexcept { ++refCount_; }
        void removeRef() noexcept {
            if (--refCount_ == 0) {
                scheduleForDeletion();
            }
        }
        [[nodiscard]] std::uint32_t refCount() const noexcept { return refCount_.load(); }

        // Traversal for cycle detection and tracing GC
        virtual void traverse(std::function<void(GCObject*)> visitor) = 0;

        // Size calculation for memory management
        [[nodiscard]] virtual Size objectSize() const noexcept = 0;

    protected:
        virtual void scheduleForDeletion();

    private:
        GCHeader header_;
        std::atomic<std::uint32_t> refCount_{0};

        friend class GCManager;
        template <typename T>
        friend class GCPtr;
    };

    /**
     * @brief Smart pointer for GC-managed objects with cycle detection
     *
     * Optimized replacement for std::shared_ptr with:
     * - Lower overhead atomic operations
     * - Integrated cycle detection
     * - Weak reference support
     * - Thread-safe reference counting
     */
    template <typename T>
    class GCPtr {
    public:
        using element_type = T;
        using pointer = T*;
        using reference = T&;

        // Constructors
        constexpr GCPtr() noexcept = default;
        constexpr explicit GCPtr(std::nullptr_t) noexcept;
        explicit GCPtr(T* ptr) noexcept;

        // Copy semantics
        GCPtr(const GCPtr& other) noexcept;
        template <typename U>
            requires std::convertible_to<U*, T*>
        explicit GCPtr(const GCPtr<U>& other) noexcept;

        // Move semantics
        GCPtr(GCPtr&& other) noexcept;
        template <typename U>
            requires std::convertible_to<U*, T*>
        explicit GCPtr(GCPtr<U>&& other) noexcept;

        // Assignment operators
        GCPtr& operator=(const GCPtr& other) noexcept;
        GCPtr& operator=(GCPtr&& other) noexcept;
        GCPtr& operator=(std::nullptr_t) noexcept;

        // Destructor
        ~GCPtr();

        // Access operators
        [[nodiscard]] T& operator*() const noexcept;
        [[nodiscard]] T* operator->() const noexcept;
        [[nodiscard]] T* get() const noexcept;

        // Boolean conversion
        [[nodiscard]] explicit operator bool() const noexcept;

        // Comparison operators (inline implementations for templates)
        [[nodiscard]] bool operator==(const GCPtr& other) const noexcept {
            return ptr_ == other.ptr_;
        }
        [[nodiscard]] bool operator!=(const GCPtr& other) const noexcept {
            return ptr_ != other.ptr_;
        }
        [[nodiscard]] bool operator<(const GCPtr& other) const noexcept {
            return ptr_ < other.ptr_;
        }

        // Utility methods
        void reset() noexcept;
        void reset(T* ptr) noexcept;
        [[nodiscard]] Size use_count() const noexcept;
        [[nodiscard]] bool unique() const noexcept;

        // Weak reference creation
        [[nodiscard]] WeakGCPtr<T> weak() const noexcept;

    private:
        T* ptr_ = nullptr;

        template <typename U>
        friend class GCPtr;
        template <typename U>
        friend class WeakGCPtr;
    };

    /**
     * @brief Weak pointer for GC-managed objects
     *
     * Provides non-owning references that don't affect reference counting.
     * Used for breaking cycles and implementing weak references.
     */
    template <typename T>
    class WeakGCPtr {
    public:
        using element_type = T;

        // Constructors
        constexpr WeakGCPtr() noexcept = default;
        explicit WeakGCPtr(const GCPtr<T>& ptr) noexcept;
        template <typename U>
            requires std::convertible_to<U*, T*>
        explicit WeakGCPtr(const GCPtr<U>& ptr) noexcept;

        // Copy and move semantics
        WeakGCPtr(const WeakGCPtr&) = default;
        WeakGCPtr& operator=(const WeakGCPtr&) = default;
        WeakGCPtr(WeakGCPtr&&) noexcept = default;
        WeakGCPtr& operator=(WeakGCPtr&&) noexcept = default;

        // Lock to get strong reference
        [[nodiscard]] GCPtr<T> lock() const noexcept;

        // Check if object still exists
        [[nodiscard]] bool expired() const noexcept;

        // Reset
        void reset() noexcept;

        // Comparison
        [[nodiscard]] bool operator==(const WeakGCPtr& other) const noexcept;

    private:
        T* ptr_ = nullptr;
    };

    /**
     * @brief Cycle detection and collection strategies
     */
    enum class GCStrategy : std::uint8_t {
        REFERENCE_COUNTING = 0,  // Pure reference counting (current)
        HYBRID_RC_TRACING = 1,   // Reference counting with cycle detection
        MARK_AND_SWEEP = 2,      // Traditional mark-and-sweep
        GENERATIONAL = 3,        // Generational GC (future)
        INCREMENTAL = 4          // Incremental GC (future)
    };

    /**
     * @brief GC statistics for monitoring and tuning
     */
    struct GCStats {
        Size totalAllocated = 0;
        Size totalFreed = 0;
        Size currentObjects = 0;
        Size cyclesDetected = 0;
        Size collectionsRun = 0;
        std::chrono::nanoseconds totalCollectionTime{0};
        std::chrono::nanoseconds lastCollectionTime{0};
    };

    /**
     * @brief Advanced garbage collector with cycle detection
     *
     * Implements a hybrid approach:
     * 1. Primary: Optimized reference counting
     * 2. Secondary: Periodic cycle detection for circular references
     * 3. Future: Interface for tracing GC migration
     */
    class AdvancedGarbageCollector : public GarbageCollector {
    public:
        explicit AdvancedGarbageCollector(GCStrategy strategy = GCStrategy::HYBRID_RC_TRACING);
        ~AdvancedGarbageCollector() override = default;

        // Non-copyable, non-movable
        AdvancedGarbageCollector(const AdvancedGarbageCollector&) = delete;
        AdvancedGarbageCollector& operator=(const AdvancedGarbageCollector&) = delete;
        AdvancedGarbageCollector(AdvancedGarbageCollector&&) = delete;
        AdvancedGarbageCollector& operator=(AdvancedGarbageCollector&&) = delete;

        // GarbageCollector interface
        void collect() override;
        void mark_phase() override;
        void sweep_phase() override;
        void add_root(void* ptr) override;
        void remove_root(void* ptr) override;
        void add_root(GCObject* obj) override;
        void remove_root(GCObject* obj) override;
        void setMemoryManager(RuntimeMemoryManager* manager) noexcept override;
        void requestCollection() noexcept override;
        void emergencyCollection() override;
        void setCollectionThreshold(Size threshold) noexcept override;
        [[nodiscard]] bool isCollecting() const noexcept override;
        [[nodiscard]] Size objectCount() const noexcept override;
        [[nodiscard]] Size memoryUsage() const noexcept override;

        // Advanced features
        void setStrategy(GCStrategy strategy) noexcept;
        [[nodiscard]] GCStrategy strategy() const noexcept;
        void setCycleDetectionThreshold(Size threshold) noexcept;
        void setCollectionInterval(std::chrono::milliseconds interval) noexcept override;

        // Statistics and monitoring
        [[nodiscard]] const GCStats& stats() const noexcept;
        void resetStats() noexcept;

        // Manual cycle detection
        Size detectCycles();
        void breakCycles();

        // Memory pressure handling
        void handleMemoryPressure();
        void setMemoryPressureThreshold(Size threshold) noexcept;

    private:
        GCStrategy strategy_;
        Size cycleDetectionThreshold_;
        Size memoryPressureThreshold_;
        std::chrono::milliseconds collectionInterval_;

        GCStats stats_;
        std::unordered_set<void*> roots_;
        std::unordered_set<GCObject*> allObjects_;
        mutable std::mutex gcMutex_;

        // Cycle detection implementation
        void performCycleDetection();
        bool isInCycle(GCObject* obj);
        void markReachableFromRoots();
        void sweepUnmarkedObjects();
    };

    /**
     * @brief Factory functions for creating GC-managed objects
     */
    template <typename T, typename... Args>
    [[nodiscard]] GCPtr<T> makeGCObject(Args&&... args);

    // Internal helper for GC registration
    namespace detail {
        void registerWithGC(GCObject* obj);
    }

    /**
     * @brief RAII GC root manager
     *
     * Automatically manages GC roots with RAII semantics.
     * Ensures roots are properly added/removed from the GC.
     */
    template <typename T>
    class GCRoot {
    public:
        explicit GCRoot(GCPtr<T> ptr, GarbageCollector& gc);
        ~GCRoot();

        // Non-copyable, movable
        GCRoot(const GCRoot&) = delete;
        GCRoot& operator=(const GCRoot&) = delete;
        GCRoot(GCRoot&& other) noexcept;
        GCRoot& operator=(GCRoot&& other) noexcept;

        // Access
        [[nodiscard]] T& operator*() const noexcept;
        [[nodiscard]] T* operator->() const noexcept;
        [[nodiscard]] T* get() const noexcept;
        [[nodiscard]] const GCPtr<T>& ptr() const noexcept;

    private:
        GCPtr<T> ptr_;
        GarbageCollector& gc_;
    };

    /**
     * @brief Default garbage collector implementation
     *
     * Maintains backward compatibility while providing
     * the new advanced GC features.
     */
    class DefaultGarbageCollector : public AdvancedGarbageCollector {
    public:
        DefaultGarbageCollector();
    };

    // Template implementations

    // GCPtr template method implementations
    template <typename T>
    constexpr GCPtr<T>::GCPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

    template <typename T>
    GCPtr<T>::GCPtr(T* ptr) noexcept : ptr_(ptr) {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->addRef();
            }
        }
    }

    template <typename T>
    GCPtr<T>::GCPtr(const GCPtr& other) noexcept : ptr_(other.ptr_) {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->addRef();
            }
        }
    }

    template <typename T>
    template <typename U>
        requires std::convertible_to<U*, T*>
    GCPtr<T>::GCPtr(const GCPtr<U>& other) noexcept : ptr_(other.ptr_) {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->addRef();
            }
        }
    }

    template <typename T>
    GCPtr<T>::GCPtr(GCPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    template <typename T>
    template <typename U>
        requires std::convertible_to<U*, T*>
    GCPtr<T>::GCPtr(GCPtr<U>&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    template <typename T>
    GCPtr<T>& GCPtr<T>::operator=(const GCPtr& other) noexcept {
        if (this != &other) {
            if constexpr (std::derived_from<T, GCObject>) {
                if (ptr_) {
                    static_cast<GCObject*>(ptr_)->removeRef();
                }
            }
            ptr_ = other.ptr_;
            if constexpr (std::derived_from<T, GCObject>) {
                if (ptr_) {
                    static_cast<GCObject*>(ptr_)->addRef();
                }
            }
        }
        return *this;
    }

    template <typename T>
    GCPtr<T>& GCPtr<T>::operator=(GCPtr&& other) noexcept {
        if (this != &other) {
            if constexpr (std::derived_from<T, GCObject>) {
                if (ptr_) {
                    static_cast<GCObject*>(ptr_)->removeRef();
                }
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    template <typename T>
    GCPtr<T>& GCPtr<T>::operator=(std::nullptr_t) noexcept {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->removeRef();
                ptr_ = nullptr;
            }
        } else {
            ptr_ = nullptr;
        }
        return *this;
    }

    template <typename T>
    GCPtr<T>::~GCPtr() {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->removeRef();
            }
        }
    }

    template <typename T>
    T& GCPtr<T>::operator*() const noexcept {
        return *ptr_;
    }

    template <typename T>
    T* GCPtr<T>::operator->() const noexcept {
        return ptr_;
    }

    template <typename T>
    T* GCPtr<T>::get() const noexcept {
        return ptr_;
    }

    template <typename T>
    GCPtr<T>::operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    template <typename T>
    void GCPtr<T>::reset() noexcept {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->removeRef();
                ptr_ = nullptr;
            }
        } else {
            ptr_ = nullptr;
        }
    }

    template <typename T>
    void GCPtr<T>::reset(T* ptr) noexcept {
        if constexpr (std::derived_from<T, GCObject>) {
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->removeRef();
            }
            ptr_ = ptr;
            if (ptr_) {
                static_cast<GCObject*>(ptr_)->addRef();
            }
        } else {
            ptr_ = ptr;
        }
    }

    template <typename T>
    Size GCPtr<T>::use_count() const noexcept {
        if constexpr (std::derived_from<T, GCObject>) {
            return ptr_ ? static_cast<GCObject*>(ptr_)->refCount() : 0;
        } else {
            return ptr_ ? 1 : 0;  // Non-GC objects always have count 1 if they exist
        }
    }

    template <typename T>
    bool GCPtr<T>::unique() const noexcept {
        return use_count() == 1;
    }

    template <typename T>
    WeakGCPtr<T> GCPtr<T>::weak() const noexcept {
        return WeakGCPtr<T>(*this);
    }

    // WeakGCPtr template method implementations
    template <typename T>
    WeakGCPtr<T>::WeakGCPtr(const GCPtr<T>& ptr) noexcept : ptr_(ptr.get()) {}

    template <typename T>
    template <typename U>
        requires std::convertible_to<U*, T*>
    WeakGCPtr<T>::WeakGCPtr(const GCPtr<U>& ptr) noexcept : ptr_(ptr.get()) {}

    template <typename T>
    GCPtr<T> WeakGCPtr<T>::lock() const noexcept {
        if constexpr (std::derived_from<T, GCObject>) {
            // TODO For now, we can't safely check if the object is still alive
            // without a proper weak reference system that tracks object lifetime
            // Return nullptr to indicate the object is no longer available
            return GCPtr<T>(nullptr);
        } else {
            // For non-GC objects, just return the pointer if it exists
            if (ptr_) {
                return GCPtr<T>(ptr_);
            }
        }
        return GCPtr<T>(nullptr);
    }

    template <typename T>
    bool WeakGCPtr<T>::expired() const noexcept {
        if constexpr (std::derived_from<T, GCObject>) {
            // TODO For now, we'll assume the object is expired if we don't have a pointer
            // In a real implementation, we'd need a proper weak reference system
            // that tracks object lifetime
            return !ptr_;
        } else {
            return !ptr_;  // Non-GC objects are never "expired" in the GC sense
        }
    }

    template <typename T>
    void WeakGCPtr<T>::reset() noexcept {
        ptr_ = nullptr;
    }

    template <typename T>
    bool WeakGCPtr<T>::operator==(const WeakGCPtr& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    // GCRoot template method implementations
    template <typename T>
    GCRoot<T>::GCRoot(GCPtr<T> ptr, GarbageCollector& gc) : ptr_(std::move(ptr)), gc_(gc) {
        if (ptr_) {
            gc_.add_root(ptr_.get());
        }
    }

    template <typename T>
    GCRoot<T>::~GCRoot() {
        if (ptr_) {
            gc_.remove_root(ptr_.get());
        }
    }

    template <typename T>
    GCRoot<T>::GCRoot(GCRoot&& other) noexcept : ptr_(std::move(other.ptr_)), gc_(other.gc_) {
        other.ptr_.reset();
    }

    template <typename T>
    GCRoot<T>& GCRoot<T>::operator=(GCRoot&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                gc_.remove_root(ptr_.get());
            }
            ptr_ = std::move(other.ptr_);
            other.ptr_.reset();
        }
        return *this;
    }

    template <typename T>
    T& GCRoot<T>::operator*() const noexcept {
        return *ptr_;
    }

    template <typename T>
    T* GCRoot<T>::operator->() const noexcept {
        return ptr_.get();
    }

    template <typename T>
    T* GCRoot<T>::get() const noexcept {
        return ptr_.get();
    }

    template <typename T>
    const GCPtr<T>& GCRoot<T>::ptr() const noexcept {
        return ptr_;
    }

    // Factory function implementation
    template <typename T, typename... Args>
    GCPtr<T> makeGCObject(Args&&... args) {
        static_assert(std::derived_from<T, GCObject>, "T must derive from GCObject");

        T* obj = new T(std::forward<Args>(args)...);
        detail::registerWithGC(static_cast<GCObject*>(obj));
        return GCPtr<T>(obj);
    }

}  // namespace rangelua::runtime
