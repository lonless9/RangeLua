/**
 * @file function.cpp
 * @brief Implementation of comprehensive Function API for RangeLua
 * @version 0.1.0
 */

#include <rangelua/api/function.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

#include <stdexcept>

namespace rangelua::api {

    namespace {
        auto& logger() {
            static auto log = utils::Logger::create_logger("api.function");
            return log;
        }
    }  // namespace

    // Construction and conversion
    Function::Function(runtime::Value value) {
        if (value.is_function()) {
            auto result = value.to_function();
            if (std::holds_alternative<runtime::Value::FunctionPtr>(result)) {
                function_ = std::get<runtime::Value::FunctionPtr>(result);
                logger()->debug("Created function wrapper from Value");
            } else {
                logger()->error("Failed to extract function from Value");
                throw std::invalid_argument("Failed to extract function from Value");
            }
        } else {
            logger()->error("Attempted to create Function from non-function Value");
            throw std::invalid_argument("Value is not a function");
        }
    }

    Function::Function(runtime::GCPtr<runtime::Function> function) : function_(std::move(function)) {
        if (!function_) {
            logger()->error("Attempted to create Function from null GCPtr");
            throw std::invalid_argument("Function pointer is null");
        }
        logger()->debug("Created function wrapper from GCPtr");
    }

    Function::Function(std::function<std::vector<runtime::Value>(const std::vector<runtime::Value>&)> fn)
        : function_(runtime::makeGCObject<runtime::Function>(std::move(fn))) {
        logger()->debug("Created function wrapper from C++ callable");
    }

    // Validation
    bool Function::is_valid() const noexcept {
        return function_.get() != nullptr;
    }

    bool Function::is_function() const noexcept {
        return is_valid();
    }

    // Function properties
    Function::Type Function::type() const {
        ensure_valid();
        auto runtime_type = function_->type();
        switch (runtime_type) {
            case runtime::Function::Type::C_FUNCTION:
                return Type::C_FUNCTION;
            case runtime::Function::Type::LUA_FUNCTION:
                return Type::LUA_FUNCTION;
            case runtime::Function::Type::CLOSURE:
                return Type::CLOSURE;
            default:
                return Type::C_FUNCTION;
        }
    }

    Size Function::parameter_count() const noexcept {
        if (!is_valid()) return 0;
        return function_->parameterCount();
    }

    Size Function::upvalue_count() const noexcept {
        if (!is_valid()) return 0;
        return function_->upvalueCount();
    }

    bool Function::is_c_function() const noexcept {
        if (!is_valid()) return false;
        return function_->isCFunction();
    }

    bool Function::is_lua_function() const noexcept {
        if (!is_valid()) return false;
        return function_->isLuaFunction();
    }

    bool Function::is_closure() const noexcept {
        if (!is_valid()) return false;
        return function_->isClosure();
    }

    // Function calling
    Result<std::vector<runtime::Value>> Function::call() const {
        ensure_valid();
        try {
            auto result = function_->call(std::vector<runtime::Value>{});
            return result;
        } catch (const std::exception& e) {
            logger()->error("Function call failed: {}", e.what());
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Result<std::vector<runtime::Value>> Function::call(const std::vector<runtime::Value>& args) const {
        ensure_valid();
        try {
            auto result = function_->call(args);
            return result;
        } catch (const std::exception& e) {
            logger()->error("Function call failed: {}", e.what());
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    // Upvalue management
    runtime::Value Function::get_upvalue(Size index) const {
        ensure_valid();
        return function_->getUpvalueValue(index);
    }

    void Function::set_upvalue(Size index, const runtime::Value& value) {
        ensure_valid();
        function_->setUpvalueValue(index, value);
    }

    void Function::add_upvalue(const runtime::Value& value) {
        ensure_valid();
        function_->addUpvalue(value);
    }

    // Bytecode access
    std::vector<Instruction> Function::get_bytecode() const {
        ensure_valid();
        if (is_lua_function()) {
            return function_->bytecode();
        }
        return {};
    }

    std::vector<runtime::Value> Function::get_constants() const {
        ensure_valid();
        // Note: This would need to be implemented in the runtime::Function class
        // For now, return empty vector
        return {};
    }

    // Debug information
    String Function::get_name() const {
        ensure_valid();
        // Note: This would need to be implemented in the runtime::Function class
        return "function";
    }

    String Function::get_source() const {
        ensure_valid();
        // Note: This would need to be implemented in the runtime::Function class
        return "<unknown>";
    }

    Size Function::get_line_number() const {
        ensure_valid();
        // Note: This would need to be implemented in the runtime::Function class
        return 0;
    }

    // Conversion and access
    runtime::Value Function::to_value() const {
        ensure_valid();
        return runtime::Value(function_);
    }

    runtime::GCPtr<runtime::Function> Function::get_function() const {
        return function_;
    }

    const runtime::Function& Function::operator*() const {
        ensure_valid();
        return *function_;
    }

    const runtime::Function* Function::operator->() const {
        ensure_valid();
        return function_.get();
    }

    // Comparison
    bool Function::operator==(const Function& other) const {
        return function_ == other.function_;
    }

    bool Function::operator!=(const Function& other) const {
        return !(*this == other);
    }

    // Function call operator
    Result<std::vector<runtime::Value>> Function::operator()(const std::vector<runtime::Value>& args) const {
        return call(args);
    }

    // Private methods
    void Function::ensure_valid() const {
        if (!is_valid()) {
            logger()->error("Operation on invalid function");
            throw std::runtime_error("Function is not valid");
        }
    }

    std::vector<runtime::Value> Function::convert_args() const {
        return {};
    }

    // Template method implementations
    template<typename... Args>
    Result<std::vector<runtime::Value>> Function::call(Args&&... args) const {
        auto converted_args = convert_args(std::forward<Args>(args)...);
        return call(converted_args);
    }

    template<typename R>
    Result<R> Function::call_single() const {
        auto result = call();
        if (std::holds_alternative<std::vector<runtime::Value>>(result)) {
            auto values = std::get<std::vector<runtime::Value>>(result);
            if (!values.empty()) {
                if constexpr (std::same_as<R, runtime::Value>) {
                    return values[0];
                } else {
                    auto converted = values[0].template to<R>();
                    if (std::holds_alternative<R>(converted)) {
                        return std::get<R>(converted);
                    }
                    return ErrorCode::TYPE_ERROR;
                }
            }
            if constexpr (std::same_as<R, runtime::Value>) {
                return runtime::Value{};
            } else {
                return ErrorCode::TYPE_ERROR;
            }
        }
        return std::get<ErrorCode>(result);
    }

    template<typename R, typename... Args>
    Result<R> Function::call_single(Args&&... args) const {
        auto result = call(std::forward<Args>(args)...);
        if (std::holds_alternative<std::vector<runtime::Value>>(result)) {
            auto values = std::get<std::vector<runtime::Value>>(result);
            if (!values.empty()) {
                if constexpr (std::same_as<R, runtime::Value>) {
                    return values[0];
                } else {
                    auto converted = values[0].template to<R>();
                    if (std::holds_alternative<R>(converted)) {
                        return std::get<R>(converted);
                    }
                    return ErrorCode::TYPE_ERROR;
                }
            }
            if constexpr (std::same_as<R, runtime::Value>) {
                return runtime::Value{};
            } else {
                return ErrorCode::TYPE_ERROR;
            }
        }
        return std::get<ErrorCode>(result);
    }

    template<typename... Args>
    Result<std::vector<runtime::Value>> Function::operator()(Args&&... args) const {
        return call(std::forward<Args>(args)...);
    }

    template<typename T>
    std::vector<runtime::Value> Function::convert_args(T&& arg) const {
        std::vector<runtime::Value> result;
        if constexpr (std::same_as<std::decay_t<T>, runtime::Value>) {
            result.push_back(std::forward<T>(arg));
        } else {
            result.push_back(runtime::Value(std::forward<T>(arg)));
        }
        return result;
    }

    template<typename T, typename... Rest>
    std::vector<runtime::Value> Function::convert_args(T&& arg, Rest&&... rest) const {
        auto result = convert_args(std::forward<T>(arg));
        auto rest_args = convert_args(std::forward<Rest>(rest)...);
        result.insert(result.end(), rest_args.begin(), rest_args.end());
        return result;
    }

    // Factory functions
    namespace function_factory {

        Function from_c_function(std::function<std::vector<runtime::Value>(const std::vector<runtime::Value>&)> fn) {
            return Function(std::move(fn));
        }

        template<typename Callable>
        Function from_callable(Callable&& callable) {
            auto wrapper = [callable = std::forward<Callable>(callable)]([[maybe_unused]] const std::vector<runtime::Value>& args) -> std::vector<runtime::Value> {
                // This is a simplified wrapper - in a real implementation,
                // we would need more sophisticated argument conversion
                if constexpr (std::is_invocable_v<Callable>) {
                    if constexpr (std::same_as<std::invoke_result_t<Callable>, void>) {
                        callable();
                        return {};
                    } else {
                        auto result = callable();
                        return {runtime::Value(result)};
                    }
                } else {
                    // For now, just call with no arguments
                    return {};
                }
            };
            return Function(wrapper);
        }

        template<typename Lambda>
        Function from_lambda(Lambda&& lambda) {
            return from_callable(std::forward<Lambda>(lambda));
        }

    }  // namespace function_factory

}  // namespace rangelua::api
