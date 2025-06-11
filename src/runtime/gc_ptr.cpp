/**
 * @file gc_ptr.cpp
 * @brief Smart pointer implementations for GC-managed objects
 * @version 0.2.0
 */

#include <rangelua/runtime/gc.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::runtime {

    // Factory function implementation
    namespace detail {
        // Helper for object creation with proper GC registration
        void registerWithGC(GCObject* obj) {
            if (obj) {
                // Register with the thread-local garbage collector
                auto gc_result = getGarbageCollector();
                if (is_success(gc_result)) {
                    auto* gc = get_value(gc_result);
                    // Cast to AdvancedGarbageCollector to access trackObject
                    if (auto* advanced_gc = dynamic_cast<AdvancedGarbageCollector*>(gc)) {
                        advanced_gc->trackObject(obj);
                        GC_LOG_DEBUG("GC object created and tracked: {} (type: {})",
                                     static_cast<void*>(obj),
                                     static_cast<int>(obj->type()));
                    }
                } else {
                    // If no GC is available, just log the creation
                    GC_LOG_ERROR("GC object created but no GC available to track it: {} (type: {})",
                                 static_cast<void*>(obj),
                                 static_cast<int>(obj->type()));
                }
            }
        }
    } // namespace detail

    // Note: Template implementations for GCPtr are in the header file,
    // as it no longer contains complex logic requiring a .cpp file.
    // This file now only contains the detail helper function.

}  // namespace rangelua::runtime
