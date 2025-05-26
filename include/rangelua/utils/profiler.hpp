#pragma once

/**
 * @file profiler.hpp
 * @brief Performance profiling utilities
 * @version 0.1.0
 */

#include <chrono>

#include "../core/types.hpp"

namespace rangelua::utils {

    /**
     * @brief Simple profiler for performance measurement
     */
    class Profiler {
    public:
        /**
         * @brief Start profiling
         */
        static void start(const String& name);

        /**
         * @brief End profiling
         */
        static void end(const String& name);

        /**
         * @brief Get profiling results
         */
        static std::unordered_map<String, std::chrono::nanoseconds> results();

    private:
        static std::unordered_map<String, std::chrono::high_resolution_clock::time_point>
            start_times_;
        static std::unordered_map<String, std::chrono::nanoseconds> durations_;
    };

}  // namespace rangelua::utils
