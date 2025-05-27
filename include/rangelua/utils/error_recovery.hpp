/**
 * @file error_recovery.hpp
 * @brief Advanced error recovery strategies and utilities
 * @version 0.1.0
 */

#pragma once

#include "../core/error.hpp"
#include "../core/types.hpp"
#include "../core/config.hpp"

#include <functional>
#include <chrono>
#include <thread>
#include <vector>
#include <optional>

namespace rangelua::utils {

    /**
     * @brief Error recovery strategy interface
     */
    template<typename T>
    class ErrorRecoveryStrategy {
    public:
        virtual ~ErrorRecoveryStrategy() = default;

        /**
         * @brief Attempt to recover from an error
         * @param error The error that occurred
         * @param context Additional context information
         * @return Recovery result or nullopt if recovery failed
         */
        virtual std::optional<T> recover(ErrorCode error, StringView context) = 0;

        /**
         * @brief Check if this strategy can handle the given error
         */
        virtual bool can_handle(ErrorCode error) const = 0;
    };

    /**
     * @brief Retry strategy for transient errors
     */
    template<typename T>
    class RetryStrategy : public ErrorRecoveryStrategy<T> {
    public:
        explicit RetryStrategy(Size max_attempts = 3,
                             std::chrono::milliseconds delay = std::chrono::milliseconds(100))
            : max_attempts_(max_attempts), delay_(delay) {}

        std::optional<T> recover(ErrorCode error, StringView context) override {
            if (!can_handle(error)) {
                return std::nullopt;
            }

            for (Size attempt = 0; attempt < max_attempts_; ++attempt) {
                if (attempt > 0) {
                    std::this_thread::sleep_for(delay_ * (1 << attempt)); // Exponential backoff
                }

                // Attempt recovery (this would be customized per use case)
                if (attempt_recovery()) {
                    return create_default_value();
                }
            }

            return std::nullopt;
        }

        bool can_handle(ErrorCode error) const override {
            return error == ErrorCode::IO_ERROR ||
                   error == ErrorCode::MEMORY_ERROR ||
                   error == ErrorCode::COROUTINE_ERROR;
        }

    private:
        Size max_attempts_;
        std::chrono::milliseconds delay_;

        virtual bool attempt_recovery() { return false; }
        virtual T create_default_value() = 0;
    };

    /**
     * @brief Fallback strategy that provides default values
     */
    template<typename T>
    class FallbackStrategy : public ErrorRecoveryStrategy<T> {
    public:
        explicit FallbackStrategy(T fallback_value) : fallback_value_(std::move(fallback_value)) {}

        std::optional<T> recover(ErrorCode error, StringView context) override {
            if (can_handle(error)) {
                return fallback_value_;
            }
            return std::nullopt;
        }

        bool can_handle(ErrorCode error) const override {
            return error != ErrorCode::STACK_OVERFLOW; // Don't fallback for critical errors
        }

    private:
        T fallback_value_;
    };

    /**
     * @brief Circuit breaker pattern for error handling
     */
    class CircuitBreaker {
    public:
        enum class State { CLOSED, OPEN, HALF_OPEN };

        explicit CircuitBreaker(Size failure_threshold = 5,
                               std::chrono::seconds timeout = std::chrono::seconds(60))
            : failure_threshold_(failure_threshold), timeout_(timeout),
              failure_count_(0), state_(State::CLOSED) {}

        /**
         * @brief Execute operation with circuit breaker protection
         */
        template<typename F>
        auto execute(F&& operation) -> decltype(operation()) {
            if (state_ == State::OPEN) {
                if (should_attempt_reset()) {
                    state_ = State::HALF_OPEN;
                } else {
                    throw RuntimeError("Circuit breaker is OPEN");
                }
            }

            try {
                auto result = std::forward<F>(operation)();
                on_success();
                return result;
            } catch (...) {
                on_failure();
                throw;
            }
        }

        State get_state() const { return state_; }
        Size get_failure_count() const { return failure_count_; }

    private:
        Size failure_threshold_;
        std::chrono::seconds timeout_;
        Size failure_count_;
        State state_;
        std::chrono::steady_clock::time_point last_failure_time_;

        bool should_attempt_reset() {
            auto now = std::chrono::steady_clock::now();
            return (now - last_failure_time_) >= timeout_;
        }

        void on_success() {
            failure_count_ = 0;
            state_ = State::CLOSED;
        }

        void on_failure() {
            failure_count_++;
            last_failure_time_ = std::chrono::steady_clock::now();

            if (failure_count_ >= failure_threshold_) {
                state_ = State::OPEN;
            }
        }
    };

    /**
     * @brief Error recovery manager that coordinates multiple strategies
     */
    template<typename T>
    class ErrorRecoveryManager {
    public:
        using StrategyPtr = std::unique_ptr<ErrorRecoveryStrategy<T>>;

        void add_strategy(StrategyPtr strategy) {
            strategies_.push_back(std::move(strategy));
        }

        /**
         * @brief Attempt recovery using available strategies
         */
        std::optional<T> attempt_recovery(ErrorCode error, StringView context = "") {
            for (auto& strategy : strategies_) {
                if (strategy->can_handle(error)) {
                    auto result = strategy->recover(error, context);
                    if (result.has_value()) {
                        return result;
                    }
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Execute operation with automatic error recovery
         */
        template<typename F>
        Result<T> execute_with_recovery(F&& operation, StringView context = "") {
            try {
                return make_success(std::forward<F>(operation)());
            } catch (const Exception& ex) {
                auto recovery = attempt_recovery(ex.code(), context);
                if (recovery.has_value()) {
                    return make_success(std::move(*recovery));
                }
                return make_error<T>(ex.code());
            }
        }

    private:
        std::vector<StrategyPtr> strategies_;
    };

    /**
     * @brief RAII error context for better error reporting
     */
    class ErrorContext {
    public:
        explicit ErrorContext(String context) : context_(std::move(context)) {
            push_context(context_);
        }

        ~ErrorContext() {
            pop_context();
        }

        static String get_current_context() {
            if (context_stack_.empty()) {
                return "";
            }

            String full_context;
            for (const auto& ctx : context_stack_) {
                if (!full_context.empty()) {
                    full_context += " -> ";
                }
                full_context += ctx;
            }
            return full_context;
        }

        // Non-copyable, non-movable
        ErrorContext(const ErrorContext&) = delete;
        ErrorContext& operator=(const ErrorContext&) = delete;
        ErrorContext(ErrorContext&&) = delete;
        ErrorContext& operator=(ErrorContext&&) = delete;

    private:
        String context_;
        static thread_local std::vector<String> context_stack_;

        static void push_context(const String& context) {
            context_stack_.push_back(context);
        }

        static void pop_context() {
            if (!context_stack_.empty()) {
                context_stack_.pop_back();
            }
        }
    };

    // Convenience macros for error context
    #define RANGELUA_ERROR_CONTEXT(name) \
        rangelua::utils::ErrorContext _error_ctx_##__LINE__(name)

    #define RANGELUA_ERROR_CONTEXT_FUNC() \
        RANGELUA_ERROR_CONTEXT(__PRETTY_FUNCTION__)

}  // namespace rangelua::utils
