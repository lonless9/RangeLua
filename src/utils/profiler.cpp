/**
 * @file profiler.cpp
 * @brief Advanced profiler implementation
 * @version 0.1.0
 */

#include <rangelua/utils/profiler.hpp>
#include <rangelua/utils/logger.hpp>
#include <rangelua/utils/debug.hpp>

#include <numeric>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace rangelua::utils {

    // PerformanceMetrics implementation
    String PerformanceMetrics::to_string() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);

        oss << "Calls: " << call_count;
        oss << ", Total: " << (total_time.count() / 1000000.0) << "ms";
        oss << ", Avg: " << (avg_time.count() / 1000000.0) << "ms";
        oss << ", Min: " << (min_time.count() / 1000000.0) << "ms";
        oss << ", Max: " << (max_time.count() / 1000000.0) << "ms";

        if (memory_allocated > 0 || memory_deallocated > 0) {
            oss << ", Mem: +" << Debug::format_memory_size(memory_allocated);
            oss << "/-" << Debug::format_memory_size(memory_deallocated);
        }

        return oss.str();
    }

    // Profiler static members
    std::mutex Profiler::mutex_;
    std::unordered_map<String, std::chrono::high_resolution_clock::time_point> Profiler::start_times_;
    std::unordered_map<String, PerformanceMetrics> Profiler::metrics_;
    std::atomic<bool> Profiler::enabled_{true};

    void Profiler::start(const String& name) {
        if (!enabled_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        start_times_[name] = std::chrono::high_resolution_clock::now();
    }

    void Profiler::end(const String& name) {
        if (!enabled_) return;

        auto end_time = std::chrono::high_resolution_clock::now();

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = start_times_.find(name);
        if (it != start_times_.end()) {
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - it->second);

            metrics_[name].update(duration);
            start_times_.erase(it);
        }
    }

    void Profiler::record_allocation(const String& context, Size bytes) {
        if (!enabled_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[context].update(std::chrono::nanoseconds{0}, static_cast<int64_t>(bytes));
    }

    void Profiler::record_deallocation(const String& context, Size bytes) {
        if (!enabled_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[context].update(std::chrono::nanoseconds{0}, -static_cast<int64_t>(bytes));
    }

    std::optional<PerformanceMetrics> Profiler::get_metrics(const String& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::unordered_map<String, PerformanceMetrics> Profiler::get_all_metrics() {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }

    void Profiler::clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        start_times_.clear();
        metrics_.clear();
    }

    String Profiler::generate_report() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (metrics_.empty()) {
            return "No profiling data available.\n";
        }

        std::ostringstream oss;
        oss << "=== Performance Report ===\n";
        oss << std::left << std::setw(40) << "Function/Section"
            << std::setw(60) << "Metrics" << "\n";
        oss << std::string(100, '-') << "\n";

        // Sort by total time
        std::vector<std::pair<String, PerformanceMetrics>> sorted_metrics(
            metrics_.begin(), metrics_.end());

        std::sort(sorted_metrics.begin(), sorted_metrics.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.total_time > b.second.total_time;
                  });

        for (const auto& [name, metrics] : sorted_metrics) {
            oss << std::left << std::setw(40) << name
                << std::setw(60) << metrics.to_string() << "\n";
        }

        oss << std::string(100, '-') << "\n";

        // Summary statistics
        auto total_calls = std::accumulate(sorted_metrics.begin(), sorted_metrics.end(), 0ULL,
                                         [](Size sum, const auto& pair) {
                                             return sum + pair.second.call_count;
                                         });

        auto total_time = std::accumulate(sorted_metrics.begin(), sorted_metrics.end(),
                                        std::chrono::nanoseconds{0},
                                        [](auto sum, const auto& pair) {
                                            return sum + pair.second.total_time;
                                        });

        oss << "Total Functions: " << sorted_metrics.size() << "\n";
        oss << "Total Calls: " << total_calls << "\n";
        oss << "Total Time: " << (total_time.count() / 1000000.0) << "ms\n";

        return oss.str();
    }

    // MemoryProfiler static members
    std::mutex MemoryProfiler::mutex_;
    std::unordered_map<void*, MemoryProfiler::AllocationInfo> MemoryProfiler::allocations_;
    std::atomic<Size> MemoryProfiler::current_usage_{0};
    std::atomic<Size> MemoryProfiler::peak_usage_{0};
    std::atomic<Size> MemoryProfiler::allocation_count_{0};

    void MemoryProfiler::record_allocation(void* ptr, Size size, const String& context) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = {size, std::chrono::steady_clock::now(), context};

        current_usage_ += size;
        peak_usage_ = std::max(peak_usage_.load(), current_usage_.load());
        allocation_count_++;
    }

    void MemoryProfiler::record_deallocation(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            current_usage_ -= it->second.size;
            allocations_.erase(it);
        }
    }

    Size MemoryProfiler::get_current_usage() {
        return current_usage_;
    }

    Size MemoryProfiler::get_peak_usage() {
        return peak_usage_;
    }

    Size MemoryProfiler::get_allocation_count() {
        return allocation_count_;
    }

    String MemoryProfiler::generate_report() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream oss;
        oss << "=== Memory Usage Report ===\n";
        oss << "Current Usage: " << Debug::format_memory_size(current_usage_) << "\n";
        oss << "Peak Usage: " << Debug::format_memory_size(peak_usage_) << "\n";
        oss << "Total Allocations: " << allocation_count_ << "\n";
        oss << "Active Allocations: " << allocations_.size() << "\n";

        if (!allocations_.empty()) {
            oss << "\nActive Allocations:\n";
            oss << std::left << std::setw(20) << "Address"
                << std::setw(15) << "Size"
                << std::setw(30) << "Context" << "\n";
            oss << std::string(65, '-') << "\n";

            for (const auto& [ptr, info] : allocations_) {
                oss << std::left << std::setw(20) << ptr
                    << std::setw(15) << Debug::format_memory_size(info.size)
                    << std::setw(30) << info.context << "\n";
            }
        }

        return oss.str();
    }

    void MemoryProfiler::clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.clear();
        current_usage_ = 0;
        peak_usage_ = 0;
        allocation_count_ = 0;
    }

    // PerformanceMonitor static members
    std::atomic<bool> PerformanceMonitor::monitoring_{false};
    std::unique_ptr<std::thread> PerformanceMonitor::monitor_thread_;

    void PerformanceMonitor::start_monitoring(std::chrono::milliseconds interval,
                                             MetricsCallback callback) {
        if (monitoring_) {
            return; // Already monitoring
        }

        monitoring_ = true;
        monitor_thread_ = std::make_unique<std::thread>([interval, callback]() {
            while (monitoring_) {
                std::this_thread::sleep_for(interval);
                if (monitoring_) {
                    auto metrics = Profiler::get_all_metrics();
                    callback(metrics);
                }
            }
        });
    }

    void PerformanceMonitor::stop_monitoring() {
        monitoring_ = false;
        if (monitor_thread_ && monitor_thread_->joinable()) {
            monitor_thread_->join();
            monitor_thread_.reset();
        }
    }

    bool PerformanceMonitor::is_monitoring() {
        return monitoring_;
    }

}  // namespace rangelua::utils
