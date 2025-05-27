/**
 * @file api.cpp
 * @brief Implementation of main API functions for RangeLua
 * @version 0.1.0
 */

#include <rangelua/api/api.hpp>
#include <rangelua/utils/logger.hpp>

#include <iostream>
#include <stdexcept>

namespace rangelua::api {

    namespace {
        auto& logger() {
            static auto log = utils::Logger::create_logger("api");
            return log;
        }

        bool initialized = false;
    }  // namespace

    // API lifecycle
    void initialize() {
        if (initialized) {
            logger()->warn("RangeLua API already initialized");
            return;
        }

        logger()->info("Initializing RangeLua API v{}", Version::STRING);

        // Initialize subsystems here
        // TODO: Initialize memory manager, GC, etc.

        initialized = true;
        logger()->info("RangeLua API initialization complete");
    }

    void cleanup() {
        if (!initialized) {
            logger()->warn("RangeLua API not initialized");
            return;
        }

        logger()->info("Cleaning up RangeLua API");

        // Cleanup subsystems here
        // TODO: Cleanup memory manager, GC, etc.

        initialized = false;
        logger()->info("RangeLua API cleanup complete");
    }

    const Version& version() noexcept {
        static const Version v;
        return v;
    }

    // Convenience functions
    namespace convenience {

        template<typename T>
        Result<T> eval(const String& code) {
            auto state = create_state();
            auto result = state.execute(code);
            if (std::holds_alternative<std::vector<runtime::Value>>(result)) {
                auto values = std::get<std::vector<runtime::Value>>(result);
                if (!values.empty()) {
                    if constexpr (std::same_as<T, runtime::Value>) {
                        return values[0];
                    } else {
                        auto converted = values[0].template to<T>();
                        if (std::holds_alternative<T>(converted)) {
                            return std::get<T>(converted);
                        }
                        return ErrorCode::TYPE_ERROR;
                    }
                }
                if constexpr (std::same_as<T, runtime::Value>) {
                    return runtime::Value{};
                } else {
                    return ErrorCode::TYPE_ERROR;
                }
            }
            return std::get<ErrorCode>(result);
        }

        template<typename T>
        Result<T> eval_file(const String& filename) {
            auto state = create_state();
            auto result = state.execute_file(filename);
            if (std::holds_alternative<std::vector<runtime::Value>>(result)) {
                auto values = std::get<std::vector<runtime::Value>>(result);
                if (!values.empty()) {
                    if constexpr (std::same_as<T, runtime::Value>) {
                        return values[0];
                    } else {
                        auto converted = values[0].template to<T>();
                        if (std::holds_alternative<T>(converted)) {
                            return std::get<T>(converted);
                        }
                        return ErrorCode::TYPE_ERROR;
                    }
                }
                if constexpr (std::same_as<T, runtime::Value>) {
                    return runtime::Value{};
                } else {
                    return ErrorCode::TYPE_ERROR;
                }
            }
            return std::get<ErrorCode>(result);
        }

        State create_state() {
            return State();
        }

        State create_state(const StateConfig& config) {
            return State(config);
        }

        Table create_table(std::initializer_list<std::pair<runtime::Value, runtime::Value>> init) {
            return Table(init);
        }

        template<typename Callable>
        Function create_function(Callable&& callable) {
            return function_factory::from_callable(std::forward<Callable>(callable));
        }

        Coroutine create_coroutine() {
            return coroutine_factory::create();
        }

        Coroutine create_coroutine(Size stack_size) {
            return coroutine_factory::create(stack_size);
        }

        Userdata create_userdata(Size size) {
            return userdata_factory::create(size);
        }

        template<typename T>
        Userdata create_userdata(T&& object) {
            return userdata_factory::from_move(std::forward<T>(object));
        }

    }  // namespace convenience

    // Type conversion utilities
    namespace convert {

        template<typename T>
        runtime::Value to_lua(T&& value) {
            if constexpr (std::same_as<std::decay_t<T>, runtime::Value>) {
                return std::forward<T>(value);
            } else {
                return runtime::Value(std::forward<T>(value));
            }
        }

        template<typename T>
        Result<T> from_lua(const runtime::Value& value) {
            if constexpr (std::same_as<T, runtime::Value>) {
                return value;
            } else {
                return value.template to<T>();
            }
        }

        template<typename Container>
        Table container_to_table(const Container& container) {
            auto table = Table();
            Size index = 1;
            for (const auto& item : container) {
                table.set_array(index++, to_lua(item));
            }
            return table;
        }

        template<typename Container>
        Result<Container> table_to_container(const Table& table) {
            Container result;
            Size size = table.array_size();

            if constexpr (requires { result.reserve(size); }) {
                result.reserve(size);
            }

            for (Size i = 1; i <= size; ++i) {
                auto value = table.get_array(i);
                if (!value.is_nil()) {
                    using ValueType = typename Container::value_type;
                    auto converted = from_lua<ValueType>(value);
                    if (std::holds_alternative<ValueType>(converted)) {
                        if constexpr (requires { result.push_back(std::get<ValueType>(converted)); }) {
                            result.push_back(std::get<ValueType>(converted));
                        } else if constexpr (requires { result.insert(std::get<ValueType>(converted)); }) {
                            result.insert(std::get<ValueType>(converted));
                        }
                    } else {
                        return std::get<ErrorCode>(converted);
                    }
                }
            }

            return result;
        }

    }  // namespace convert

    // Error handling utilities
    namespace error {

        String to_string(ErrorCode code) {
            switch (code) {
                case ErrorCode::SUCCESS:
                    return "Success";
                case ErrorCode::SYNTAX_ERROR:
                    return "Syntax error";
                case ErrorCode::RUNTIME_ERROR:
                    return "Runtime error";
                case ErrorCode::MEMORY_ERROR:
                    return "Memory error";
                case ErrorCode::TYPE_ERROR:
                    return "Type error";
                case ErrorCode::ARGUMENT_ERROR:
                    return "Argument error";
                case ErrorCode::STACK_OVERFLOW:
                    return "Stack overflow";
                case ErrorCode::COROUTINE_ERROR:
                    return "Coroutine error";
                case ErrorCode::IO_ERROR:
                    return "I/O error";
                case ErrorCode::UNKNOWN_ERROR:
                default:
                    return "Unknown error";
            }
        }

        template<typename T>
        bool is_success(const Result<T>& result) {
            return std::holds_alternative<T>(result);
        }

        template<typename T>
        const T& get_value(const Result<T>& result) {
            if (std::holds_alternative<T>(result)) {
                return std::get<T>(result);
            }
            throw std::runtime_error("Result contains error: " + to_string(std::get<ErrorCode>(result)));
        }

        template<typename T>
        ErrorCode get_error(const Result<T>& result) {
            if (std::holds_alternative<ErrorCode>(result)) {
                return std::get<ErrorCode>(result);
            }
            throw std::runtime_error("Result contains success value, not error");
        }

        template<typename T>
        T get_value_or(const Result<T>& result, T&& default_value) {
            if (std::holds_alternative<T>(result)) {
                return std::get<T>(result);
            }
            return std::forward<T>(default_value);
        }

    }  // namespace error

    // Debugging utilities
    namespace debug {

        String value_to_string(const runtime::Value& value) {
            return value.debug_string();
        }

        String value_type_name(const runtime::Value& value) {
            return value.type_name();
        }

        void print_value(const runtime::Value& value) {
            std::cout << value_to_string(value) << std::endl;
        }

        void dump_table(const Table& table, Size max_depth) {
            std::cout << "Table contents (max depth: " << max_depth << "):\n";

            // Print array part
            Size array_size = table.array_size();
            if (array_size > 0) {
                std::cout << "  Array part:\n";
                for (Size i = 1; i <= array_size; ++i) {
                    auto value = table.get_array(i);
                    std::cout << "    [" << i << "] = " << value_to_string(value) << "\n";
                }
            }

            // Print hash part (simplified - would need iterator support)
            std::cout << "  Hash part: " << table.hash_size() << " entries\n";
            std::cout << "  Total size: " << table.total_size() << "\n";
        }

    }  // namespace debug

}  // namespace rangelua::api
