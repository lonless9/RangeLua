#pragma once

/**
 * @file gc.hpp
 * @brief Modern C++20 Tracing Garbage Collector for RangeLua - Declarations Only
 * @version 0.2.0
 *
 * This file contains declarations for a standard tracing garbage collector.
 * It moves away from a complex hybrid system to a robust mark-and-sweep
 * collector, which correctly handles cycles and finalizers.
 *
 * Design:
 * - Tri-color Mark-and-Sweep algorithm.
 * - No more reference counting in GCObject or GCPtr.
 * - GCObjects are tracked in a single 'allgc' list.
 * - Finalizers ('__gc') are handled correctly.
 * - Weak references are handled by weak tables (WeakGCPtr removed).
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
    class AdvancedGarbageCollector;
    template <typename T>
    class GCPtr;
    class RuntimeMemoryManager;
    class Value;

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
     * @brief GC object concepts for type safety
     */
    template <typename T>
    concept GCManaged = requires(T* obj, AdvancedGarbageCollector& gc) {
        { obj->header_ } -> std::convertible_to<GCHeader&>;
        { obj->traverse(gc) } -> std::same_as<void>;
    };

    /**
     * @brief Enhanced GC object header with modern C++20 patterns
     */
    struct GCHeader {
        GCObject* next = nullptr;
        LuaType type;
        lu_byte marked = 0;

        explicit constexpr GCHeader(LuaType t) noexcept : type(t), marked(0) {}

        // Modern C++20 comparison operators
        constexpr auto operator<=>(const GCHeader&) const noexcept = default;
    };

    /**
     * @brief Base class for all garbage-collected objects.
     *
     * This version removes reference counting to align with a tracing GC model.
     * Objects are linked in a single list and their lifetime is managed by the collector.
     */
    class GCObject {
    public:
        GCHeader header_;

        explicit GCObject(LuaType type) noexcept : header_(type) {}
        virtual ~GCObject() = default;

        // Non-copyable, non-movable (GC objects have identity)
        GCObject(const GCObject&) = delete;
        GCObject& operator=(const GCObject&) = delete;
        GCObject(GCObject&&) = delete;
        GCObject& operator=(GCObject&&) = delete;

        // GC interface
        [[nodiscard]] LuaType type() const noexcept { return header_.type; }
        [[nodiscard]] lu_byte getMarked() const noexcept { return header_.marked; }
        void setMarked(lu_byte m) noexcept { header_.marked = m; }

        /**
         * @brief Traverses the object to mark its children.
         * @param gc The garbage collector instance.
         */
        virtual void traverse(AdvancedGarbageCollector& gc) = 0;

        /**
         * @brief Calculates the memory size of the object.
         */
        [[nodiscard]] virtual Size objectSize() const noexcept = 0;

        friend class AdvancedGarbageCollector;
    };

    /**
     * @brief Smart pointer for GC-managed objects.
     *
     * This is a simple handle to a GC-managed object. It does not perform
     * reference counting. The object's lifetime is managed by the tracing GC.
     */
    template <typename T>
    class GCPtr {
    public:
        using element_type = T;
        using pointer = T*;
        using reference = T&;

        // Constructors
        constexpr GCPtr() noexcept = default;
        constexpr explicit GCPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}
        explicit GCPtr(T* ptr) noexcept : ptr_(ptr) {}

        // Copy and move semantics
        GCPtr(const GCPtr& other) noexcept = default;
        template <typename U>
            requires std::convertible_to<U*, T*>
        explicit GCPtr(const GCPtr<U>& other) noexcept : ptr_(other.get()) {}

        GCPtr(GCPtr&& other) noexcept = default;
        template <typename U>
            requires std::convertible_to<U*, T*>
        explicit GCPtr(GCPtr<U>&& other) noexcept : ptr_(other.get()) { other.reset(); }

        // Assignment operators
        GCPtr& operator=(const GCPtr& other) noexcept = default;
        GCPtr& operator=(GCPtr&& other) noexcept = default;
        GCPtr& operator=(std::nullptr_t) noexcept {
            ptr_ = nullptr;
            return *this;
        }

        // Destructor (does nothing, GC handles memory)
        ~GCPtr() = default;

        // Access operators
        [[nodiscard]] T& operator*() const noexcept { return *ptr_; }
        [[nodiscard]] T* operator->() const noexcept { return ptr_; }
        [[nodiscard]] T* get() const noexcept { return ptr_; }

        // Boolean conversion
        [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

        // Comparison operators
        [[nodiscard]] bool operator==(const GCPtr& other) const noexcept { return ptr_ == other.ptr_; }
        [[nodiscard]] auto operator<=>(const GCPtr& other) const noexcept = default;

        // Utility methods
        void reset() noexcept { ptr_ = nullptr; }
        void reset(T* ptr) noexcept { ptr_ = ptr; }
        [[nodiscard]] long use_count() const noexcept { return ptr_ ? 1 : 0; } // Simplified
        [[nodiscard]] bool unique() const noexcept { return use_count() == 1; }

    private:
        T* ptr_ = nullptr;
    };

    /**
     * @brief GC statistics for monitoring and tuning
     */
    struct GCStats {
        Size totalAllocated = 0;
        Size totalFreed = 0;
        Size currentObjects = 0;
        Size collectionsRun = 0;
        std::chrono::nanoseconds totalCollectionTime{0};
        std::chrono::nanoseconds lastCollectionTime{0};
    };

    /**
     * @brief A standard tracing garbage collector.
     *
     * Implements a tri-color mark-and-sweep algorithm.
     */
    class AdvancedGarbageCollector : public GarbageCollector {
    public:
        explicit AdvancedGarbageCollector();
        ~AdvancedGarbageCollector() override;

        // Non-copyable, non-movable
        AdvancedGarbageCollector(const AdvancedGarbageCollector&) = delete;
        AdvancedGarbageCollector& operator=(const AdvancedGarbageCollector&) = delete;
        AdvancedGarbageCollector(AdvancedGarbageCollector&&) = delete;
        AdvancedGarbageCollector& operator=(AdvancedGarbageCollector&&) = delete;

        // Public GC interface
        void collect() override;
        void emergencyCollection() override;

        // Object and root management
        void trackObject(GCObject* obj);
        void add_root(GCObject* obj) override;
        void remove_root(GCObject* obj) override;

        // Used by GCObject::traverse to mark children
        void markObject(GCObject* obj);
        void markValue(const Value& value);

        // Configuration and monitoring
        void setCollectionThreshold(Size threshold) noexcept override;
        void setCollectionInterval(std::chrono::milliseconds interval) noexcept override;
        [[nodiscard]] const GCStats& stats() const noexcept;
        void resetStats() noexcept;

    protected:
        // Internal GC interface (from GarbageCollector)
        void mark_phase() override;
        void sweep_phase() override;
        void add_root(void* ptr) override;
        void remove_root(void* ptr) override;
        void setMemoryManager(RuntimeMemoryManager* manager) noexcept override;
        void requestCollection() noexcept override;
        [[nodiscard]] bool isCollecting() const noexcept override;
        [[nodiscard]] Size objectCount() const noexcept override;
        [[nodiscard]] Size memoryUsage() const noexcept override;

    private:
        // Tri-color marking
        void markGray(GCObject* obj);
        void propagateMark();
        void atomicStep();
        void sweep(GCObject** list);
        void callFinalizers();
        void freeAllObjects();

        std::mutex gcMutex_;
        GCStats stats_;
        Size collectionThreshold_;
        Size debt_;

        // Object lists
        GCObject* allObjects_ = nullptr;
        GCObject* gray_ = nullptr;
        GCObject* grayAgain_ = nullptr;
        GCObject* toBeFinalized_ = nullptr;
        GCObject* weak_ = nullptr;
        GCObject* allweak_ = nullptr;
        GCObject* ephemeron_ = nullptr;

        // Root set
        std::unordered_set<GCObject*> roots_;

        lu_byte currentWhite_;
        bool isSweeping_ = false;

        // VM and State interaction
        RuntimeMemoryManager* memoryManager_ = nullptr;
        std::vector<class VirtualMachine*> vms_;
    };

    namespace detail {
        void registerWithGC(GCObject* obj);
    }

    /**
     * @brief Factory function for creating GC-managed objects
     */
    template <std::derived_from<GCObject> T, typename... Args>
    [[nodiscard]] inline GCPtr<T> makeGCObject(Args&&... args) {
        T* obj = new T(std::forward<Args>(args)...);
        detail::registerWithGC(obj);
        return GCPtr<T>(obj);
    }

    /**
     * @brief Default garbage collector implementation
     */
    class DefaultGarbageCollector : public AdvancedGarbageCollector {
    public:
        DefaultGarbageCollector();
    };

}  // namespace rangelua::runtime
