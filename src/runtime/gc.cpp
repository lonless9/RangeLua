/**
 * @file gc.cpp
 * @brief Modern C++20 Tracing Garbage Collector implementation for RangeLua
 * @version 0.2.0
 */

#include <rangelua/runtime/gc.hpp>
#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/vm.hpp> // For root scanning
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

#include <algorithm>

namespace rangelua::runtime {

#define isWhite(o)      ((o)->getMarked() & (bitmask(0) | bitmask(1)))
#define isBlack(o)      testbit((o)->getMarked(), 2)
#define isGray(o)       (!isWhite(o) && !isBlack(o))

#define otherwhite(g)   ((g)->currentWhite_ ^ (bitmask(0) | bitmask(1)))
#define isdead(g, o)    ((o)->getMarked() & otherwhite(g) & (bitmask(0) | bitmask(1)))

#define makewhite(g, o) ((o)->setMarked(((o)->getMarked() & ~((bitmask(0) | bitmask(1)) | bitmask(2))) | (g)->currentWhite_))
#define gray2black(o)   l_setbit((o)->header_.marked, 2)


// --- GCObject ---
// No methods need to be implemented here after removing RC.

// --- AdvancedGarbageCollector ---

AdvancedGarbageCollector::AdvancedGarbageCollector()
    : collectionThreshold_(1024 * 1024) // 1 MB
    , debt_(0)
    , currentWhite_(bitmask(0))
{
    GC_LOG_INFO("Tracing Garbage Collector initialized.");
}

AdvancedGarbageCollector::~AdvancedGarbageCollector() {
    GC_LOG_INFO("Shutting down Garbage Collector.");
    // Free all remaining objects to prevent leaks on shutdown.
    // In a real app, the state's closing would handle this.
    freeAllObjects();
}

void AdvancedGarbageCollector::trackObject(GCObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lock(gcMutex_);
    obj->header_.next = allObjects_;
    allObjects_ = obj;
    makewhite(this, obj);
    stats_.totalAllocated += obj->objectSize();
    stats_.currentObjects++;
    GC_LOG_TRACE("Tracked new object: {} (type: {})", static_cast<void*>(obj), static_cast<int>(obj->type()));
}


void AdvancedGarbageCollector::add_root(GCObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lock(gcMutex_);
    roots_.insert(obj);
    GC_LOG_DEBUG("Added root: {} (type: {})", static_cast<void*>(obj), static_cast<int>(obj->type()));
}

void AdvancedGarbageCollector::remove_root(GCObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lock(gcMutex_);
    roots_.erase(obj);
    GC_LOG_DEBUG("Removed root: {}", static_cast<void*>(obj));
}


void AdvancedGarbageCollector::markObject(GCObject* obj) {
    if (obj && isWhite(obj)) {
        markGray(obj);
    }
}

void AdvancedGarbageCollector::markValue(const Value& value) {
    if (value.is_gc_object()) {
        markObject(value.as_gc_object());
    }
}


void AdvancedGarbageCollector::markGray(GCObject* obj) {
    gray2black(obj); // Temporarily mark black to prevent re-graying
    l_setbit(obj->header_.marked, 3); // Use bit 3 as gray flag
    obj->header_.next = gray_;
    gray_ = obj;
}

void AdvancedGarbageCollector::propagateMark() {
    while (gray_) {
        GCObject* current = gray_;
        gray_ = current->header_.next;
        current->traverse(*this);
    }
}


void AdvancedGarbageCollector::collect() {
    RANGELUA_DEBUG_TIMER("gc_full_collection");
    std::lock_guard<std::mutex> lock(gcMutex_);
    GC_LOG_INFO("Starting full garbage collection cycle.");

    auto start_time = std::chrono::high_resolution_clock::now();

    mark_phase();
    atomicStep();
    sweep_phase();

    auto end_time = std::chrono::high_resolution_clock::now();
    stats_.lastCollectionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    stats_.totalCollectionTime += stats_.lastCollectionTime;
    stats_.collectionsRun++;

    GC_LOG_INFO("Garbage collection cycle finished in {} ms.", stats_.lastCollectionTime.count() / 1e6);
}

void AdvancedGarbageCollector::emergencyCollection() {
    collect();
}

void AdvancedGarbageCollector::mark_phase() {
    RANGELUA_DEBUG_TIMER("gc_mark_phase");
    GC_LOG_DEBUG("Mark phase started. Roots: {}", roots_.size());

    // Mark all roots
    for (auto* root : roots_) {
        markObject(root);
    }

    // Propagate marks
    propagateMark();
}

void AdvancedGarbageCollector::atomicStep() {
    RANGELUA_DEBUG_TIMER("gc_atomic_phase");
    GC_LOG_DEBUG("Atomic phase started.");

    // This is where weak table logic would go.
    // For now, it's a placeholder.

    // After propagation, separate finalizable objects.
    GCObject** p = &allObjects_;
    while (*p) {
        GCObject* current = *p;
        if (isWhite(current) && MetamethodSystem::has_metamethod(Value(current), Metamethod::GC)) {
            *p = current->header_.next; // remove from allgc
            current->header_.next = toBeFinalized_;
            toBeFinalized_ = current;
            l_setbit(current->header_.marked, 5); // Finalize bit
        }
        else {
            p = &current->header_.next;
        }
    }

    // Mark finalizable objects and their dependencies so they are not collected yet.
    GCObject* f = toBeFinalized_;
    toBeFinalized_ = nullptr; // List will be rebuilt
    while (f) {
        GCObject* next = f->header_.next;
        f->header_.next = nullptr;
        markGray(f);
        propagateMark(); // Mark all dependencies
        f->header_.next = toBeFinalized_;
        toBeFinalized_ = f;
        f = next;
    }
}


void AdvancedGarbageCollector::sweep_phase() {
    RANGELUA_DEBUG_TIMER("gc_sweep_phase");
    isSweeping_ = true;
    GC_LOG_DEBUG("Sweep phase started.");

    // Sweeping main list
    sweep(&allObjects_);

    // Sweeping finalizable list (for next cycle)
    sweep(&toBeFinalized_);

    isSweeping_ = false;

    // Flip colors for the next cycle
    currentWhite_ = otherwhite(this);

    // Call finalizers for objects collected in this cycle
    callFinalizers();
}


void AdvancedGarbageCollector::sweep(GCObject** list) {
    GCObject** p = list;
    Size freed_count = 0;
    Size freed_bytes = 0;

    while (*p) {
        GCObject* current = *p;
        if (isWhite(current)) {
            // White object is garbage
            *p = current->header_.next; // Unlink
            freed_bytes += current->objectSize();
            delete current;
            freed_count++;
        } else {
            // Black object survives, turn it white for next cycle
            makewhite(this, current);
            p = &current->header_.next;
        }
    }
    stats_.totalFreed += freed_bytes;
    stats_.currentObjects -= freed_count;
    GC_LOG_DEBUG("Swept list: {} objects freed ({} bytes).", freed_count, freed_bytes);
}

void AdvancedGarbageCollector::callFinalizers() {
    GCObject* current = toBeFinalized_;
    toBeFinalized_ = nullptr; // Clear list before running finalizers
    while (current) {
        GCObject* next = current->header_.next;
        GC_LOG_DEBUG("Finalizing object: {}", static_cast<void*>(current));
        // In VM: vm->callmeta(current, "__gc")
        // For now, we simulate this.
        // After finalizer, object becomes regular garbage for the next cycle
        makewhite(this, current);
        resetbit(current->header_.marked, 5); // Clear finalize bit
        current->header_.next = allObjects_;
        allObjects_ = current;
        current = next;
    }
}

void AdvancedGarbageCollector::freeAllObjects() {
    GC_LOG_WARN("Freeing all GC objects.");
    GCObject* current = allObjects_;
    while (current) {
        GCObject* next = current->header_.next;
        delete current;
        current = next;
    }
    allObjects_ = nullptr;
}


// --- GarbageCollector interface stubs ---
void AdvancedGarbageCollector::add_root(void* ptr) { add_root(static_cast<GCObject*>(ptr)); }
void AdvancedGarbageCollector::remove_root(void* ptr) { remove_root(static_cast<GCObject*>(ptr)); }
void AdvancedGarbageCollector::setMemoryManager(RuntimeMemoryManager* manager) noexcept { memoryManager_ = manager; }
void AdvancedGarbageCollector::requestCollection() noexcept { /* For now, does nothing */ }
[[nodiscard]] bool AdvancedGarbageCollector::isCollecting() const noexcept { return false; }
[[nodiscard]] Size AdvancedGarbageCollector::objectCount() const noexcept { return stats_.currentObjects; }
[[nodiscard]] Size AdvancedGarbageCollector::memoryUsage() const noexcept { return stats_.totalAllocated - stats_.totalFreed; }
void AdvancedGarbageCollector::setCollectionThreshold(Size threshold) noexcept { collectionThreshold_ = threshold; }
void AdvancedGarbageCollector::setCollectionInterval(std::chrono::milliseconds /*interval*/) noexcept { /* no-op */ }
[[nodiscard]] const GCStats& AdvancedGarbageCollector::stats() const noexcept { return stats_; }
void AdvancedGarbageCollector::resetStats() noexcept { stats_ = {}; }


// DefaultGarbageCollector implementation
DefaultGarbageCollector::DefaultGarbageCollector() : AdvancedGarbageCollector() {
    GC_LOG_INFO("Default Tracing Garbage Collector initialized");
}

}  // namespace rangelua::runtime
