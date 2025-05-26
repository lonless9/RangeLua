#pragma once

/**
 * @file gc.hpp
 * @brief Garbage collection implementation
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "memory.hpp"

namespace rangelua::runtime {

    /**
     * @brief Default garbage collector implementation
     */
    class DefaultGarbageCollector : public GarbageCollector {
    public:
        void collect() override {}
        void mark_phase() override {}
        void sweep_phase() override {}
        void add_root(void* ptr) override {}
        void remove_root(void* ptr) override {}
    };

}  // namespace rangelua::runtime
