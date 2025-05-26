/**
 * @file rangelua.cpp
 * @brief RangeLua library initialization and cleanup
 * @version 0.1.0
 */

#include <rangelua/rangelua.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua {

    namespace {
        bool g_initialized = false;
    }

    bool initialize() noexcept {
        if (g_initialized) {
            return true;
        }

        try {
            // Initialize logging system
            utils::Logger::initialize();

            // TODO: Initialize other subsystems
            // - Memory manager
            // - Garbage collector
            // - Global state

            g_initialized = true;
            return true;
        } catch (...) {
            return false;
        }
    }

    void cleanup() noexcept {
        if (!g_initialized) {
            return;
        }

        try {
            // TODO: Cleanup subsystems

            // Shutdown logging
            utils::Logger::shutdown();

            g_initialized = false;
        } catch (...) {
            // Ignore cleanup errors
        }
    }

}  // namespace rangelua
