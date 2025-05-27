/**
 * @file coroutine.cpp
 * @brief Implementation of comprehensive Coroutine API for RangeLua
 * @version 0.1.0
 */

#include <rangelua/api/coroutine.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

#include <stdexcept>

namespace rangelua::api {

    namespace {
        auto& logger() {
            static auto log = utils::Logger::create_logger("api.coroutine");
            return log;
        }
    }  // namespace

    // Construction and conversion
    Coroutine::Coroutine() : coroutine_(runtime::makeGCObject<runtime::Coroutine>()) {
        logger()->debug("Created new coroutine with default stack size");
    }

    Coroutine::Coroutine(runtime::Value value) {
        if (value.is_thread()) {
            auto result = value.to_thread();
            if (std::holds_alternative<runtime::Value::ThreadPtr>(result)) {
                coroutine_ = std::get<runtime::Value::ThreadPtr>(result);
                logger()->debug("Created coroutine wrapper from Value");
            } else {
                logger()->error("Failed to extract coroutine from Value");
                throw std::invalid_argument("Failed to extract coroutine from Value");
            }
        } else {
            logger()->error("Attempted to create Coroutine from non-thread Value");
            throw std::invalid_argument("Value is not a thread");
        }
    }

    Coroutine::Coroutine(runtime::GCPtr<runtime::Coroutine> coroutine) : coroutine_(std::move(coroutine)) {
        if (!coroutine_) {
            logger()->error("Attempted to create Coroutine from null GCPtr");
            throw std::invalid_argument("Coroutine pointer is null");
        }
        logger()->debug("Created coroutine wrapper from GCPtr");
    }

    Coroutine::Coroutine(Size stack_size) : coroutine_(runtime::makeGCObject<runtime::Coroutine>(stack_size)) {
        logger()->debug("Created new coroutine with stack size: {}", stack_size);
    }

    // Validation
    bool Coroutine::is_valid() const noexcept {
        return coroutine_.get() != nullptr;
    }

    bool Coroutine::is_coroutine() const noexcept {
        return is_valid();
    }

    // Coroutine status
    Coroutine::Status Coroutine::status() const {
        ensure_valid();
        auto runtime_status = coroutine_->status();
        switch (runtime_status) {
            case runtime::Coroutine::Status::SUSPENDED:
                return Status::SUSPENDED;
            case runtime::Coroutine::Status::RUNNING:
                return Status::RUNNING;
            case runtime::Coroutine::Status::NORMAL:
                return Status::NORMAL;
            case runtime::Coroutine::Status::DEAD:
                return Status::DEAD;
            default:
                return Status::DEAD;
        }
    }

    bool Coroutine::is_suspended() const noexcept {
        if (!is_valid()) return false;
        return coroutine_->status() == runtime::Coroutine::Status::SUSPENDED;
    }

    bool Coroutine::is_running() const noexcept {
        if (!is_valid()) return false;
        return coroutine_->status() == runtime::Coroutine::Status::RUNNING;
    }

    bool Coroutine::is_normal() const noexcept {
        if (!is_valid()) return false;
        return coroutine_->status() == runtime::Coroutine::Status::NORMAL;
    }

    bool Coroutine::is_dead() const noexcept {
        if (!is_valid()) return true;
        return coroutine_->status() == runtime::Coroutine::Status::DEAD;
    }

    bool Coroutine::is_resumable() const noexcept {
        if (!is_valid()) return false;
        return coroutine_->isResumable();
    }

    // Coroutine execution
    Result<std::vector<runtime::Value>> Coroutine::resume() {
        ensure_valid();
        try {
            auto result = coroutine_->resume();
            return result;
        } catch (const std::exception& e) {
            logger()->error("Coroutine resume failed: {}", e.what());
            return ErrorCode::COROUTINE_ERROR;
        }
    }

    Result<std::vector<runtime::Value>> Coroutine::resume(const std::vector<runtime::Value>& args) {
        ensure_valid();
        try {
            auto result = coroutine_->resume(args);
            return result;
        } catch (const std::exception& e) {
            logger()->error("Coroutine resume failed: {}", e.what());
            return ErrorCode::COROUTINE_ERROR;
        }
    }

    Result<std::vector<runtime::Value>> Coroutine::yield() {
        ensure_valid();
        try {
            auto result = coroutine_->yield();
            return result;
        } catch (const std::exception& e) {
            logger()->error("Coroutine yield failed: {}", e.what());
            return ErrorCode::COROUTINE_ERROR;
        }
    }

    Result<std::vector<runtime::Value>> Coroutine::yield(const std::vector<runtime::Value>& values) {
        ensure_valid();
        try {
            auto result = coroutine_->yield(values);
            return result;
        } catch (const std::exception& e) {
            logger()->error("Coroutine yield failed: {}", e.what());
            return ErrorCode::COROUTINE_ERROR;
        }
    }

    // Stack management
    void Coroutine::push(const runtime::Value& value) {
        ensure_valid();
        coroutine_->push(value);
    }

    runtime::Value Coroutine::pop() {
        ensure_valid();
        return coroutine_->pop();
    }

    runtime::Value Coroutine::top() const {
        ensure_valid();
        return coroutine_->top();
    }

    Size Coroutine::stack_size() const noexcept {
        if (!is_valid()) return 0;
        return coroutine_->stackSize();
    }

    bool Coroutine::stack_empty() const noexcept {
        if (!is_valid()) return true;
        return coroutine_->stackEmpty();
    }

    // Error handling
    bool Coroutine::has_error() const noexcept {
        if (!is_valid()) return false;
        return coroutine_->hasError();
    }

    String Coroutine::get_error() const {
        ensure_valid();
        return coroutine_->error();
    }

    void Coroutine::set_error(const String& error) {
        ensure_valid();
        coroutine_->setError(error);
    }

    void Coroutine::clear_error() {
        ensure_valid();
        coroutine_->setError("");
    }

    // Conversion and access
    runtime::Value Coroutine::to_value() const {
        ensure_valid();
        return runtime::Value(coroutine_);
    }

    runtime::GCPtr<runtime::Coroutine> Coroutine::get_coroutine() const {
        return coroutine_;
    }

    const runtime::Coroutine& Coroutine::operator*() const {
        ensure_valid();
        return *coroutine_;
    }

    const runtime::Coroutine* Coroutine::operator->() const {
        ensure_valid();
        return coroutine_.get();
    }

    // Comparison
    bool Coroutine::operator==(const Coroutine& other) const {
        return coroutine_ == other.coroutine_;
    }

    bool Coroutine::operator!=(const Coroutine& other) const {
        return !(*this == other);
    }

    // Private methods
    void Coroutine::ensure_valid() const {
        if (!is_valid()) {
            logger()->error("Operation on invalid coroutine");
            throw std::runtime_error("Coroutine is not valid");
        }
    }

    std::vector<runtime::Value> Coroutine::convert_args() const {
        return {};
    }

    // Template method implementations
    template<typename... Args>
    Result<std::vector<runtime::Value>> Coroutine::resume(Args&&... args) {
        auto converted_args = convert_args(std::forward<Args>(args)...);
        return resume(converted_args);
    }

    template<typename... Args>
    Result<std::vector<runtime::Value>> Coroutine::yield(Args&&... args) {
        auto converted_args = convert_args(std::forward<Args>(args)...);
        return yield(converted_args);
    }

    template<typename T>
    void Coroutine::push_value(T&& value) {
        if constexpr (std::same_as<std::decay_t<T>, runtime::Value>) {
            push(std::forward<T>(value));
        } else {
            push(runtime::Value(std::forward<T>(value)));
        }
    }

    template<typename T>
    Result<T> Coroutine::pop_as() {
        auto value = pop();
        if constexpr (std::same_as<T, runtime::Value>) {
            return value;
        } else {
            auto converted = value.template to<T>();
            if (std::holds_alternative<T>(converted)) {
                return std::get<T>(converted);
            }
            return ErrorCode::TYPE_ERROR;
        }
    }

    template<typename T>
    Result<T> Coroutine::top_as() const {
        auto value = top();
        if constexpr (std::same_as<T, runtime::Value>) {
            return value;
        } else {
            auto converted = value.template to<T>();
            if (std::holds_alternative<T>(converted)) {
                return std::get<T>(converted);
            }
            return ErrorCode::TYPE_ERROR;
        }
    }

    template<typename... Args>
    Result<std::vector<runtime::Value>> Coroutine::operator()(Args&&... args) {
        return resume(std::forward<Args>(args)...);
    }

    template<typename T>
    std::vector<runtime::Value> Coroutine::convert_args(T&& arg) const {
        std::vector<runtime::Value> result;
        if constexpr (std::same_as<std::decay_t<T>, runtime::Value>) {
            result.push_back(std::forward<T>(arg));
        } else {
            result.push_back(runtime::Value(std::forward<T>(arg)));
        }
        return result;
    }

    template<typename T, typename... Rest>
    std::vector<runtime::Value> Coroutine::convert_args(T&& arg, Rest&&... rest) const {
        auto result = convert_args(std::forward<T>(arg));
        auto rest_args = convert_args(std::forward<Rest>(rest)...);
        result.insert(result.end(), rest_args.begin(), rest_args.end());
        return result;
    }

    // Factory functions
    namespace coroutine_factory {

        Coroutine create() {
            return Coroutine();
        }

        Coroutine create(Size stack_size) {
            return Coroutine(stack_size);
        }

        Coroutine from_function([[maybe_unused]] const runtime::Value& function) {
            auto coroutine = Coroutine();
            // TODO: Set up coroutine to execute the function
            // This would require integration with the VM
            return coroutine;
        }

    }  // namespace coroutine_factory

}  // namespace rangelua::api
