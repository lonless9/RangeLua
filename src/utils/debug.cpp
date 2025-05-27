/**
 * @file debug.cpp
 * @brief Debug utilities implementation with modern C++20 features
 * @version 0.1.0
 */

#include <rangelua/utils/debug.hpp>

#include <rangelua/utils/logger.hpp>
#include <rangelua/core/config.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <memory>

#ifdef __has_include
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define RANGELUA_HAS_BACKTRACE 1
#endif
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define RANGELUA_HAS_DEMANGLE 1
#endif
#endif

namespace rangelua::utils {

    namespace {
        // Thread-safe debug state
        std::mutex debug_mutex;
        std::unordered_map<std::thread::id, String> thread_names;
        std::unordered_map<String, std::chrono::high_resolution_clock::time_point> debug_timers;
        
        /**
         * @brief Get current thread name
         */
        String get_thread_name() {
            std::lock_guard<std::mutex> lock(debug_mutex);
            auto thread_id = std::this_thread::get_id();
            
            auto it = thread_names.find(thread_id);
            if (it != thread_names.end()) {
                return it->second;
            }
            
            // Generate default thread name
            std::ostringstream oss;
            oss << "Thread-" << thread_id;
            String name = oss.str();
            thread_names[thread_id] = name;
            return name;
        }
        
        /**
         * @brief Format current timestamp
         */
        String format_timestamp() {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
            oss << "." << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }
        
        /**
         * @brief Generate simple stack trace
         */
        std::vector<String> generate_debug_trace(int skip_frames = 2) {
            std::vector<String> trace;
            
#ifdef RANGELUA_HAS_BACKTRACE
            constexpr int max_frames = 20;
            std::array<void*, max_frames> buffer{};
            
            int frame_count = backtrace(buffer.data(), max_frames);
            
            struct SymbolsDeleter {
                void operator()(char** ptr) const noexcept {
                    if (ptr) {
                        std::free(static_cast<void*>(ptr));
                    }
                }
            };
            
            std::unique_ptr<char*, SymbolsDeleter> symbols{
                backtrace_symbols(buffer.data(), frame_count)
            };
            
            if (symbols) {
                for (int i = skip_frames; i < frame_count && i < skip_frames + 10; ++i) {
                    String frame = symbols.get()[i];
                    
                    // Simplify frame for debug output
                    size_t start = frame.find('(');
                    if (start != String::npos) {
                        size_t end = frame.find(')', start);
                        if (end != String::npos) {
                            frame = frame.substr(0, end + 1);
                        }
                    }
                    
                    trace.push_back(std::move(frame));
                }
            }
#else
            trace.push_back("Stack trace not available");
#endif
            
            return trace;
        }
    }

    void Debug::assert_msg(bool condition, const String& message) {
        if (!condition) {
            std::ostringstream oss;
            oss << "ASSERTION FAILED: " << message << "\n";
            oss << "Thread: " << get_thread_name() << "\n";
            oss << "Time: " << format_timestamp() << "\n";
            
            if constexpr (config::DEBUG_ENABLED) {
                auto trace = generate_debug_trace();
                if (!trace.empty()) {
                    oss << "Call Stack:\n";
                    for (size_t i = 0; i < trace.size(); ++i) {
                        oss << "  " << i << ": " << trace[i] << "\n";
                    }
                }
            }
            
            String error_msg = oss.str();
            
            // Log the assertion failure
            if (auto logger = utils::loggers::memory()) {
                logger->critical(error_msg);
            }
            
            // Print to stderr for immediate visibility
            std::cerr << error_msg << std::flush;
            
            // Terminate in debug mode
            if constexpr (config::DEBUG_ENABLED) {
                std::abort();
            }
        }
    }

    void Debug::print(const String& message) {
        if constexpr (config::DEBUG_ENABLED) {
            std::ostringstream oss;
            oss << "[DEBUG] [" << format_timestamp() << "] ";
            oss << "[" << get_thread_name() << "] ";
            oss << message;
            
            String debug_msg = oss.str();
            
            // Log the debug message
            if (auto logger = utils::loggers::memory()) {
                logger->debug(debug_msg);
            }
            
            // Also print to stdout in debug mode
            std::cout << debug_msg << std::endl;
        }
    }

    void Debug::set_thread_name(const String& name) {
        std::lock_guard<std::mutex> lock(debug_mutex);
        thread_names[std::this_thread::get_id()] = name;
    }

    void Debug::start_timer(const String& name) {
        std::lock_guard<std::mutex> lock(debug_mutex);
        debug_timers[name] = std::chrono::high_resolution_clock::now();
    }

    std::chrono::nanoseconds Debug::end_timer(const String& name) {
        std::lock_guard<std::mutex> lock(debug_mutex);
        
        auto it = debug_timers.find(name);
        if (it == debug_timers.end()) {
            return std::chrono::nanoseconds::zero();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - it->second);
        
        debug_timers.erase(it);
        return duration;
    }

    String Debug::format_memory_size(Size bytes) {
        constexpr Size KB = 1024;
        constexpr Size MB = KB * 1024;
        constexpr Size GB = MB * 1024;
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        
        if (bytes >= GB) {
            oss << static_cast<double>(bytes) / GB << " GB";
        } else if (bytes >= MB) {
            oss << static_cast<double>(bytes) / MB << " MB";
        } else if (bytes >= KB) {
            oss << static_cast<double>(bytes) / KB << " KB";
        } else {
            oss << bytes << " bytes";
        }
        
        return oss.str();
    }

    void Debug::dump_stack_trace() {
        if constexpr (config::DEBUG_ENABLED) {
            auto trace = generate_debug_trace(1);
            
            std::ostringstream oss;
            oss << "Stack Trace (from " << get_thread_name() << "):\n";
            
            for (size_t i = 0; i < trace.size(); ++i) {
                oss << "  " << std::setw(2) << i << ": " << trace[i] << "\n";
            }
            
            String trace_msg = oss.str();
            
            if (auto logger = utils::loggers::memory()) {
                logger->debug(trace_msg);
            }
            
            std::cout << trace_msg << std::flush;
        }
    }

}  // namespace rangelua::utils
