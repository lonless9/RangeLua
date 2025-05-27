#pragma once

/**
 * @file profiler.hpp
 * @brief Advanced performance profiling and monitoring utilities
 * @version 0.1.0
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../core/config.hpp"
#include "../core/types.hpp"

namespace rangelua::utils {

    /**
     * @brief Performance metrics data structure
     */
    struct PerformanceMetrics {
        std::chrono::nanoseconds total_time{0};
        std::chrono::nanoseconds min_time{std::chrono::nanoseconds::max()};
        std::chrono::nanoseconds max_time{0};
        std::chrono::nanoseconds avg_time{0};
        Size call_count = 0;
        Size memory_allocated = 0;
        Size memory_deallocated = 0;

        void update(std::chrono::nanoseconds duration, int64_t memory_delta = 0) {
            total_time += duration;
            min_time = std::min(min_time, duration);
            max_time = std::max(max_time, duration);
            call_count++;
            avg_time = total_time / call_count;

            if (memory_delta > 0) {
                memory_allocated += static_cast<Size>(memory_delta);
            } else if (memory_delta < 0) {
                memory_deallocated += static_cast<Size>(-memory_delta);
            }
        }

        [[nodiscard]] String to_string() const;
    };

    /**
     * @brief Advanced profiler with hierarchical timing and memory tracking
     */
    class Profiler {
    public:
        /**
         * @brief Start profiling a named section
         */
        static void start(const String& name);

        /**
         * @brief End profiling a named section
         */
        static void end(const String& name);

        /**
         * @brief Record a memory allocation
         */
        static void record_allocation(const String& context, Size bytes);

        /**
         * @brief Record a memory deallocation
         */
        static void record_deallocation(const String& context, Size bytes);

        /**
         * @brief Get profiling results for a specific section
         */
        static std::optional<PerformanceMetrics> get_metrics(const String& name);

        /**
         * @brief Get all profiling results
         */
        static std::unordered_map<String, PerformanceMetrics> get_all_metrics();

        /**
         * @brief Clear all profiling data
         */
        static void clear();

        /**
         * @brief Generate a performance report
         */
        static String generate_report();

        /**
         * @brief Enable/disable profiling
         */
        static void set_enabled(bool enabled) { enabled_ = enabled; }
        static bool is_enabled() { return enabled_; }

    private:
        static std::mutex mutex_;
        static std::unordered_map<String, std::chrono::high_resolution_clock::time_point>
            start_times_;
        static std::unordered_map<String, PerformanceMetrics> metrics_;
        static std::atomic<bool> enabled_;
    };

    /**
     * @brief RAII profiler for automatic timing
     */
    class ScopedProfiler {
    public:
        explicit ScopedProfiler(String name) : name_(std::move(name)) {
            if (Profiler::is_enabled()) {
                Profiler::start(name_);
            }
        }

        ~ScopedProfiler() {
            if (Profiler::is_enabled()) {
                Profiler::end(name_);
            }
        }

        // Non-copyable, non-movable
        ScopedProfiler(const ScopedProfiler&) = delete;
        ScopedProfiler& operator=(const ScopedProfiler&) = delete;
        ScopedProfiler(ScopedProfiler&&) = delete;
        ScopedProfiler& operator=(ScopedProfiler&&) = delete;

    private:
        String name_;
    };

    /**
     * @brief Memory profiler for tracking allocations
     */
    class MemoryProfiler {
    public:
        struct AllocationInfo {
            Size size;
            std::chrono::steady_clock::time_point timestamp;
            String context;
        };

        /**
         * @brief Record an allocation
         */
        static void record_allocation(void* ptr, Size size, const String& context = "");

        /**
         * @brief Record a deallocation
         */
        static void record_deallocation(void* ptr);

        /**
         * @brief Get current memory usage
         */
        static Size get_current_usage();

        /**
         * @brief Get peak memory usage
         */
        static Size get_peak_usage();

        /**
         * @brief Get allocation count
         */
        static Size get_allocation_count();

        /**
         * @brief Generate memory report
         */
        static String generate_report();

        /**
         * @brief Clear all tracking data
         */
        static void clear();

    private:
        static std::mutex mutex_;
        static std::unordered_map<void*, AllocationInfo> allocations_;
        static std::atomic<Size> current_usage_;
        static std::atomic<Size> peak_usage_;
        static std::atomic<Size> allocation_count_;
    };

    /**
     * @brief Performance monitor for real-time metrics
     */
    class PerformanceMonitor {
    public:
        using MetricsCallback =
            std::function<void(const std::unordered_map<String, PerformanceMetrics>&)>;

        /**
         * @brief Start monitoring with callback
         */
        static void start_monitoring(std::chrono::milliseconds interval, MetricsCallback callback);

        /**
         * @brief Stop monitoring
         */
        static void stop_monitoring();

        /**
         * @brief Check if monitoring is active
         */
        static bool is_monitoring();

    private:
        static std::atomic<bool> monitoring_;
        static std::unique_ptr<std::thread> monitor_thread_;
    };

}  // namespace rangelua::utils

// Profiling macros
#if RANGELUA_DEBUG
#    define RANGELUA_PROFILE(name)       rangelua::utils::ScopedProfiler _prof_##__LINE__(name)
#    define RANGELUA_PROFILE_FUNCTION()  RANGELUA_PROFILE(__PRETTY_FUNCTION__)
#    define RANGELUA_PROFILE_SCOPE(name) RANGELUA_PROFILE(name)
#else
#    define RANGELUA_PROFILE(name)       ((void)0)
#    define RANGELUA_PROFILE_FUNCTION()  ((void)0)
#    define RANGELUA_PROFILE_SCOPE(name) ((void)0)
#endif
