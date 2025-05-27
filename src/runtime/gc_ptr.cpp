/**
 * @file gc_ptr.cpp
 * @brief Smart pointer implementations for GC-managed objects
 * @version 0.1.0
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
                    gc->add_root(obj);
                    GC_LOG_DEBUG("GC object created and registered: {} (type: {})",
                                 static_cast<void*>(obj),
                                 static_cast<int>(obj->type()));
                } else {
                    // If no GC is available, just log the creation
                    GC_LOG_DEBUG("GC object created (no GC available): {} (type: {})",
                                 static_cast<void*>(obj),
                                 static_cast<int>(obj->type()));
                }
            }
        }
    }

    // Note: Template implementations are in the header file
    // This file contains only non-template helper functions

}  // namespace rangelua::runtime
