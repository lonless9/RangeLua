/**
 * @file rangelua.cpp
 * @brief RangeLua library initialization with modern C++20 patterns
 * @version 0.1.0
 */

#include <rangelua/rangelua.hpp>
#include <rangelua/runtime/memory.hpp>
#include <rangelua/utils/logger.hpp>

#include <mutex>

namespace rangelua {

    namespace {
        /**
         * @brief Thread-safe initialization context without global state
         */
        class InitializationContext {
        public:
            InitializationContext() = default;
            ~InitializationContext() = default;

            // Non-copyable, non-movable (singleton pattern)
            InitializationContext(const InitializationContext&) = delete;
            InitializationContext& operator=(const InitializationContext&) = delete;
            InitializationContext(InitializationContext&&) = delete;
            InitializationContext& operator=(InitializationContext&&) = delete;

            [[nodiscard]] Status initialize() noexcept {
                std::lock_guard<std::mutex> lock(mutex_);

                if (initialized_) {
                    return make_success();
                }

                try {
                    // Initialize logging system
                    auto logger_result = initialize_logging();
                    if (is_error(logger_result)) {
                        return logger_result;
                    }

                    // Initialize memory management
                    auto memory_result = initialize_memory();
                    if (is_error(memory_result)) {
                        return memory_result;
                    }

                    initialized_ = true;
                    return make_success();
                } catch (...) {
                    return make_error<std::monostate>(ErrorCode::UNKNOWN_ERROR);
                }
            }

            void cleanup() noexcept {
                std::lock_guard<std::mutex> lock(mutex_);

                if (!initialized_) {
                    return;
                }

                try {
                    // Cleanup in reverse order
                    memory_manager_.reset();
                    utils::Logger::shutdown();
                    initialized_ = false;
                } catch (...) {
                    // Ignore cleanup errors but ensure state is reset
                    initialized_ = false;
                }
            }

            [[nodiscard]] bool is_initialized() const noexcept {
                std::lock_guard<std::mutex> lock(mutex_);
                return initialized_;
            }

            [[nodiscard]] runtime::MemoryManager* memory_manager() const noexcept {
                std::lock_guard<std::mutex> lock(mutex_);
                return memory_manager_.get();
            }

        private:
            mutable std::mutex mutex_;
            bool initialized_ = false;
            UniquePtr<runtime::MemoryManager> memory_manager_;

            [[nodiscard]] Status initialize_logging() noexcept {
                try {
                    utils::Logger::initialize();
                    return make_success();
                } catch (...) {
                    return make_error<std::monostate>(ErrorCode::UNKNOWN_ERROR);
                }
            }

            [[nodiscard]] Status initialize_memory() noexcept {
                try {
                    memory_manager_ = runtime::MemoryManagerFactory::create_system_manager();
                    if (!memory_manager_) {
                        return make_error<std::monostate>(ErrorCode::MEMORY_ERROR);
                    }
                    return make_success();
                } catch (...) {
                    return make_error<std::monostate>(ErrorCode::MEMORY_ERROR);
                }
            }
        };

        // Thread-safe singleton pattern without global state issues
        InitializationContext& get_initialization_context() {
            static InitializationContext context;
            return context;
        }

    }  // anonymous namespace

    Status initialize() noexcept {
        return get_initialization_context().initialize();
    }

    void cleanup() noexcept {
        get_initialization_context().cleanup();
    }

    bool is_initialized() noexcept {
        return get_initialization_context().is_initialized();
    }

    runtime::MemoryManager* get_memory_manager() noexcept {
        return get_initialization_context().memory_manager();
    }

}  // namespace rangelua
