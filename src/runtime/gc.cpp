/**
 * @file gc.cpp
 * @brief Modern C++20 garbage collection system implementation for RangeLua with error handling and
 * debug integration
 * @version 0.1.0
 */

#include <rangelua/core/error.hpp>
#include <rangelua/runtime/gc.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

#include <algorithm>
#include <chrono>
#include <queue>
#include <unordered_set>

namespace rangelua::runtime {

    // GCObject implementation
    void GCObject::scheduleForDeletion() {
        // RANGELUA_DEBUG_PRINT("GCObject::scheduleForDeletion - object type: " +
        //                      std::to_string(static_cast<int>(type())) +
        //                      ", ref count: " + std::to_string(refCount()));

        // In a real implementation, this would add the object to a deletion queue
        // For now, we'll delete immediately to prevent memory leaks
        GC_LOG_DEBUG("GCObject scheduled for deletion: type={}", static_cast<int>(type()));

        try {
            // Try to remove from thread-local GC if available
            auto gc_result = getGarbageCollector();
            if (is_success(gc_result)) {
                auto* gc = get_value(gc_result);
                gc->remove_root(this);
                // RANGELUA_DEBUG_PRINT("Successfully removed object from GC roots");
            } else {
                ErrorCode error = get_error(gc_result);
                log_error(error, "Failed to get garbage collector during object deletion");
            }
        } catch (const std::exception& e) {
            log_error(ErrorCode::RUNTIME_ERROR, "Exception during GC cleanup: " + String(e.what()));
        }

        // RANGELUA_DEBUG_PRINT("Deleting GCObject immediately");
        delete this;
    }

    // AdvancedGarbageCollector implementation
    AdvancedGarbageCollector::AdvancedGarbageCollector(GCStrategy strategy)
        : strategy_(strategy)
        , cycleDetectionThreshold_(1000)
        , memoryPressureThreshold_(64 * 1024 * 1024)  // 64MB
        , collectionInterval_(std::chrono::milliseconds(100))
        , stats_{}
        , roots_{}
        , allObjects_{}
        , gcMutex_{} {
        GC_LOG_INFO("AdvancedGarbageCollector initialized with strategy: {}",
                     static_cast<int>(strategy_));
    }

    void AdvancedGarbageCollector::collect() {
        RANGELUA_DEBUG_TIMER("gc_collection");

        std::lock_guard<std::mutex> lock(gcMutex_);

        auto start_time = std::chrono::high_resolution_clock::now();
        GC_LOG_DEBUG("Starting garbage collection cycle");
        // RANGELUA_DEBUG_PRINT("GC collection started with strategy: " +
        //                      std::to_string(static_cast<int>(strategy_)));

        Size objects_before = allObjects_.size();
        // RANGELUA_DEBUG_PRINT("Objects before collection: " + std::to_string(objects_before));

        try {
            switch (strategy_) {
                case GCStrategy::REFERENCE_COUNTING:
                    // Pure reference counting - objects are deleted immediately when refcount
                    // reaches 0 No additional collection needed
                    // RANGELUA_DEBUG_PRINT(
                    //     "Using reference counting strategy - no additional work needed");
                    break;

                case GCStrategy::HYBRID_RC_TRACING:
                    // Hybrid approach: reference counting + cycle detection
                    // RANGELUA_DEBUG_PRINT("Using hybrid RC+tracing strategy");
                    performCycleDetection();
                    break;

                case GCStrategy::MARK_AND_SWEEP:
                    // RANGELUA_DEBUG_PRINT("Using mark-and-sweep strategy");
                    mark_phase();
                    sweep_phase();
                    break;

                default:
                    GC_LOG_WARN("Unsupported GC strategy: {}", static_cast<int>(strategy_));
                    log_error(ErrorCode::RUNTIME_ERROR,
                              "Unsupported GC strategy: " +
                                  std::to_string(static_cast<int>(strategy_)));
                    break;
            }
        } catch (const std::exception& e) {
            log_error(ErrorCode::RUNTIME_ERROR,
                      "Exception during garbage collection: " + String(e.what()));
            GC_LOG_ERROR("Exception during garbage collection: {}", e.what());
            throw;
        }

        Size objects_after = allObjects_.size();
        [[maybe_unused]] Size objects_collected =
            objects_before >= objects_after ? objects_before - objects_after : 0;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto collection_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        // Update statistics
        stats_.collectionsRun++;
        stats_.lastCollectionTime = collection_time;
        stats_.totalCollectionTime += collection_time;
        stats_.currentObjects = objects_after;

        GC_LOG_DEBUG("Garbage collection completed: {} objects collected in {}ns",
                     objects_collected,
                     collection_time.count());

        // RANGELUA_DEBUG_PRINT(
        //     "GC collection completed - Objects collected: " + std::to_string(objects_collected) +
        //     ", Time: " + std::to_string(collection_time.count()) + "ns");
    }

    void AdvancedGarbageCollector::mark_phase() {
        GC_LOG_DEBUG("Starting mark phase");

        // Unmark all objects first
        for (auto* obj : allObjects_) {
            if (obj) {
                obj->unmark();
            }
        }

        // Mark all reachable objects from roots
        markReachableFromRoots();

        GC_LOG_DEBUG("Mark phase completed");
    }

    void AdvancedGarbageCollector::sweep_phase() {
        GC_LOG_DEBUG("Starting sweep phase");

        sweepUnmarkedObjects();

        GC_LOG_DEBUG("Sweep phase completed");
    }

    void AdvancedGarbageCollector::add_root(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(gcMutex_);
        roots_.insert(ptr);
        GC_LOG_DEBUG("Added root pointer: {}", ptr);
    }

    void AdvancedGarbageCollector::remove_root(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(gcMutex_);
        roots_.erase(ptr);
        GC_LOG_DEBUG("Removed root pointer: {}", ptr);
    }

    void AdvancedGarbageCollector::add_root(GCObject* obj) {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(gcMutex_);
        roots_.insert(obj);
        allObjects_.insert(obj);
        GC_LOG_DEBUG("Added root GCObject: {} (type: {})",
                     static_cast<void*>(obj),
                     static_cast<int>(obj->type()));
    }

    void AdvancedGarbageCollector::remove_root(GCObject* obj) {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(gcMutex_);
        roots_.erase(obj);
        allObjects_.erase(obj);
        GC_LOG_DEBUG("Removed root GCObject: {} (type: {})",
                     static_cast<void*>(obj),
                     static_cast<int>(obj->type()));
    }

    void AdvancedGarbageCollector::setMemoryManager(RuntimeMemoryManager* manager) noexcept {
        // Store reference to memory manager for integration
        // In a full implementation, we'd use this for allocation tracking
        GC_LOG_DEBUG("Memory manager set: {}", static_cast<void*>(manager));
    }

    void AdvancedGarbageCollector::requestCollection() noexcept {
        // Schedule a collection to run at the next opportunity
        // For now, we'll run it immediately
        try {
            collect();
        } catch (...) {
            GC_LOG_ERROR("Exception during requested garbage collection");
        }
    }

    void AdvancedGarbageCollector::emergencyCollection() {
        GC_LOG_WARN("Emergency garbage collection triggered");

        // Force immediate collection regardless of current state
        try {
            collect();

            // If still under memory pressure, try more aggressive collection
            if (memoryUsage() > memoryPressureThreshold_) {
                handleMemoryPressure();
            }
        } catch (...) {
            GC_LOG_ERROR("Exception during emergency garbage collection");
        }
    }

    void AdvancedGarbageCollector::setCollectionThreshold(Size threshold) noexcept {
        cycleDetectionThreshold_ = threshold;
        GC_LOG_DEBUG("Collection threshold set to: {}", threshold);
    }

    bool AdvancedGarbageCollector::isCollecting() const noexcept {
        // Check if GC is currently running (would need atomic flag in real implementation)
        return false;  // Simplified for now
    }

    Size AdvancedGarbageCollector::objectCount() const noexcept {
        std::lock_guard<std::mutex> lock(gcMutex_);
        return allObjects_.size();
    }

    Size AdvancedGarbageCollector::memoryUsage() const noexcept {
        std::lock_guard<std::mutex> lock(gcMutex_);

        Size total_size = 0;
        for (const auto* obj : allObjects_) {
            if (obj) {
                total_size += obj->objectSize();
            }
        }
        return total_size;
    }

    void AdvancedGarbageCollector::setStrategy(GCStrategy strategy) noexcept {
        std::lock_guard<std::mutex> lock(gcMutex_);
        strategy_ = strategy;
        GC_LOG_INFO("GC strategy changed to: {}", static_cast<int>(strategy));
    }

    GCStrategy AdvancedGarbageCollector::strategy() const noexcept {
        return strategy_;
    }

    void AdvancedGarbageCollector::setCycleDetectionThreshold(Size threshold) noexcept {
        cycleDetectionThreshold_ = threshold;
        GC_LOG_DEBUG("Cycle detection threshold set to: {}", threshold);
    }

    void AdvancedGarbageCollector::setCollectionInterval(std::chrono::milliseconds interval) noexcept {
        collectionInterval_ = interval;
        GC_LOG_DEBUG("Collection interval set to: {}ms", interval.count());
    }

    const GCStats& AdvancedGarbageCollector::stats() const noexcept {
        return stats_;
    }

    void AdvancedGarbageCollector::resetStats() noexcept {
        std::lock_guard<std::mutex> lock(gcMutex_);
        stats_ = {};
        GC_LOG_DEBUG("GC statistics reset");
    }

    Size AdvancedGarbageCollector::detectCycles() {
        std::lock_guard<std::mutex> lock(gcMutex_);

        GC_LOG_DEBUG("Starting cycle detection");
        Size cycles_found = 0;

        // Simple cycle detection algorithm
        // In a real implementation, this would use more sophisticated algorithms
        // like the tri-color marking algorithm or Bacon-Rajan cycle collection

        for (auto* obj : allObjects_) {
            if (obj && obj->refCount() > 0 && !obj->isMarked()) {
                if (isInCycle(obj)) {
                    cycles_found++;
                    GC_LOG_DEBUG("Cycle detected involving object: {}", static_cast<void*>(obj));
                }
            }
        }

        stats_.cyclesDetected += cycles_found;
        GC_LOG_DEBUG("Cycle detection completed: {} cycles found", cycles_found);
        return cycles_found;
    }

    void AdvancedGarbageCollector::breakCycles() {
        std::lock_guard<std::mutex> lock(gcMutex_);

        GC_LOG_DEBUG("Breaking detected cycles");

        // In a real implementation, this would break cycles by:
        // 1. Identifying strongly connected components
        // 2. Breaking weak references in cycles
        // 3. Scheduling objects for deletion

        // For now, we'll just log the action
        GC_LOG_DEBUG("Cycle breaking completed");
    }

    void AdvancedGarbageCollector::handleMemoryPressure() {
        GC_LOG_WARN("Handling memory pressure");

        // Aggressive collection strategies under memory pressure
        GCStrategy original_strategy = strategy_;

        try {
            // Temporarily switch to mark-and-sweep for more thorough collection
            if (strategy_ != GCStrategy::MARK_AND_SWEEP) {
                setStrategy(GCStrategy::MARK_AND_SWEEP);
                collect();
            }

            // Force cycle detection and breaking
            detectCycles();
            breakCycles();

            // Run another collection cycle
            collect();

        } catch (...) {
            GC_LOG_ERROR("Exception during memory pressure handling");
        }

        // Restore original strategy
        setStrategy(original_strategy);

        GC_LOG_INFO("Memory pressure handling completed");
    }

    void AdvancedGarbageCollector::setMemoryPressureThreshold(Size threshold) noexcept {
        memoryPressureThreshold_ = threshold;
        GC_LOG_DEBUG("Memory pressure threshold set to: {}", threshold);
    }

    // Private implementation methods
    void AdvancedGarbageCollector::performCycleDetection() {
        GC_LOG_DEBUG("Performing cycle detection for hybrid GC");

        // Only run cycle detection if we have enough objects
        if (allObjects_.size() >= cycleDetectionThreshold_) {
            Size cycles = detectCycles();
            if (cycles > 0) {
                breakCycles();
            }
        }
    }

    bool AdvancedGarbageCollector::isInCycle(GCObject* obj) {
        if (!obj) return false;

        // Simple cycle detection using DFS
        // In a real implementation, this would be more sophisticated
        std::unordered_set<GCObject*> visited;
        std::unordered_set<GCObject*> recursion_stack;

        std::function<bool(GCObject*)> dfs = [&](GCObject* current) -> bool {
            if (!current) return false;

            if (recursion_stack.count(current)) {
                return true;  // Cycle detected
            }

            if (visited.count(current)) {
                return false;  // Already processed
            }

            visited.insert(current);
            recursion_stack.insert(current);

            // Traverse all objects referenced by this object
            bool cycle_found = false;
            current->traverse([&](GCObject* referenced) {
                if (referenced && dfs(referenced)) {
                    cycle_found = true;
                }
            });

            recursion_stack.erase(current);
            return cycle_found;
        };

        return dfs(obj);
    }

    void AdvancedGarbageCollector::markReachableFromRoots() {
        GC_LOG_DEBUG("Marking reachable objects from {} roots", roots_.size());

        std::queue<GCObject*> work_queue;

        // Add all root objects to the work queue
        for (void* root_ptr : roots_) {
            auto* gc_obj = static_cast<GCObject*>(root_ptr);
            if (gc_obj && !gc_obj->isMarked()) {
                gc_obj->mark();
                work_queue.push(gc_obj);
            }
        }

        // Process the work queue
        while (!work_queue.empty()) {
            GCObject* current = work_queue.front();
            work_queue.pop();

            // Traverse all objects referenced by the current object
            current->traverse([&](GCObject* referenced) {
                if (referenced && !referenced->isMarked()) {
                    referenced->mark();
                    work_queue.push(referenced);
                }
            });
        }

        GC_LOG_DEBUG("Marking phase completed");
    }

    void AdvancedGarbageCollector::sweepUnmarkedObjects() {
        GC_LOG_DEBUG("Sweeping unmarked objects");

        auto it = allObjects_.begin();
        [[maybe_unused]] Size swept_count = 0;

        while (it != allObjects_.end()) {
            GCObject* obj = *it;

            if (!obj || !obj->isMarked()) {
                // Object is not marked, delete it
                if (obj) {
                    GC_LOG_DEBUG("Sweeping object: {} (type: {})",
                                 static_cast<void*>(obj),
                                 static_cast<int>(obj->type()));
                    delete obj;
                    swept_count++;
                }
                it = allObjects_.erase(it);
            } else {
                // Object is marked, keep it and unmark for next cycle
                obj->unmark();
                ++it;
            }
        }

        GC_LOG_DEBUG("Sweep completed: {} objects deleted", swept_count);
    }

    // DefaultGarbageCollector implementation
    DefaultGarbageCollector::DefaultGarbageCollector()
        : AdvancedGarbageCollector(GCStrategy::HYBRID_RC_TRACING) {
        GC_LOG_INFO("DefaultGarbageCollector initialized");
    }

}  // namespace rangelua::runtime
