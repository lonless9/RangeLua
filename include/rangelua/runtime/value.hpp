#pragma once

/**
 * @file value.hpp
 * @brief Lua value system with modern C++20 features
 * @version 0.1.0
 */

#include <concepts>
#include <functional>
#include <memory>
#include <variant>

#include "../core/types.hpp"

namespace rangelua::runtime {

    // Forward declarations
    class Table;
    class Function;
    class Coroutine;
    class Userdata;

    /**
     * @brief Lua value types
     */
    enum class ValueType : std::uint8_t {
        Nil = 0,
        Boolean,
        Number,
        String,
        Table,
        Function,
        Userdata,
        Thread
    };

    // Make ValueType convertible to int for concept compatibility
    constexpr int to_int(ValueType type) noexcept {
        return static_cast<int>(type);
    }

}  // namespace rangelua::runtime

// Add conversion function in global scope for concept compatibility
namespace std {
    template <>
    struct is_convertible<rangelua::runtime::ValueType, int> : std::true_type {};
}  // namespace std

namespace rangelua::runtime {

    /**
     * @brief Lua value representation using std::variant
     */
    class Value {
    public:
        // Type alias required by LuaValue concept
        using value_type = Value;

        using Nil = std::monostate;
        using Boolean = bool;
        using Number = double;
        using String = std::string;
        using TablePtr = std::shared_ptr<Table>;
        using FunctionPtr = std::shared_ptr<Function>;
        using UserdataPtr = std::shared_ptr<Userdata>;
        using ThreadPtr = std::shared_ptr<Coroutine>;

        using ValueVariant = std::
            variant<Nil, Boolean, Number, String, TablePtr, FunctionPtr, UserdataPtr, ThreadPtr>;

        // Constructors
        Value() noexcept : data_(Nil{}) {}
        explicit Value(Nil nil_value) noexcept : data_(nil_value) {}
        explicit Value(Boolean b) noexcept : data_(b) {}
        explicit Value(Number n) noexcept : data_(n) {}
        explicit Value(Int i) noexcept : data_(static_cast<Number>(i)) {}
        explicit Value(const char* s) : data_(String(s)) {}
        explicit Value(String s) : data_(std::move(s)) {}
        explicit Value(StringView s) : data_(String(s)) {}
        explicit Value(TablePtr t) noexcept : data_(std::move(t)) {}
        explicit Value(FunctionPtr f) noexcept : data_(std::move(f)) {}
        explicit Value(UserdataPtr u) noexcept : data_(std::move(u)) {}
        explicit Value(ThreadPtr t) noexcept : data_(std::move(t)) {}

        // Copy and move semantics
        Value(const Value&) = default;
        Value(Value&&) noexcept = default;
        Value& operator=(const Value&) = default;
        Value& operator=(Value&&) noexcept = default;
        ~Value() = default;

        // Type queries
        [[nodiscard]] ValueType type() const noexcept;
        [[nodiscard]] int type_id() const noexcept { return static_cast<int>(type()); }

        // For concept compatibility - provide type() that returns int
        [[nodiscard]] int type_as_int() const noexcept { return static_cast<int>(type()); }

        [[nodiscard]] bool is_nil() const noexcept { return std::holds_alternative<Nil>(data_); }
        [[nodiscard]] bool is_boolean() const noexcept {
            return std::holds_alternative<Boolean>(data_);
        }
        [[nodiscard]] bool is_number() const noexcept {
            return std::holds_alternative<Number>(data_);
        }
        [[nodiscard]] bool is_string() const noexcept {
            return std::holds_alternative<String>(data_);
        }
        [[nodiscard]] bool is_table() const noexcept {
            return std::holds_alternative<TablePtr>(data_);
        }
        [[nodiscard]] bool is_function() const noexcept {
            return std::holds_alternative<FunctionPtr>(data_);
        }
        [[nodiscard]] bool is_userdata() const noexcept {
            return std::holds_alternative<UserdataPtr>(data_);
        }
        [[nodiscard]] bool is_thread() const noexcept {
            return std::holds_alternative<ThreadPtr>(data_);
        }

        // Type conversions with error handling
        [[nodiscard]] Result<Boolean> to_boolean() const noexcept;
        [[nodiscard]] Result<Number> to_number() const noexcept;
        [[nodiscard]] Result<String> to_string() const;
        [[nodiscard]] Result<TablePtr> to_table() const noexcept;
        [[nodiscard]] Result<FunctionPtr> to_function() const noexcept;
        [[nodiscard]] Result<UserdataPtr> to_userdata() const noexcept;
        [[nodiscard]] Result<ThreadPtr> to_thread() const noexcept;

        // Generic conversion method for concepts
        template <typename T>
        [[nodiscard]] Result<T> to() const {
            if constexpr (std::same_as<T, Boolean>) {
                return to_boolean();
            } else if constexpr (std::same_as<T, Number>) {
                return to_number();
            } else if constexpr (std::same_as<T, String>) {
                return to_string();
            } else if constexpr (std::same_as<T, TablePtr>) {
                return to_table();
            } else if constexpr (std::same_as<T, FunctionPtr>) {
                return to_function();
            } else if constexpr (std::same_as<T, UserdataPtr>) {
                return to_userdata();
            } else if constexpr (std::same_as<T, ThreadPtr>) {
                return to_thread();
            } else {
                return ErrorCode::TYPE_ERROR;
            }
        }

        // Lua truthiness
        [[nodiscard]] bool is_truthy() const noexcept;
        [[nodiscard]] bool is_falsy() const noexcept { return !is_truthy(); }

        // Comparison operators
        bool operator==(const Value& other) const noexcept;
        bool operator!=(const Value& other) const noexcept { return !(*this == other); }
        bool operator<(const Value& other) const;
        bool operator<=(const Value& other) const;
        bool operator>(const Value& other) const;
        bool operator>=(const Value& other) const;

        // Arithmetic operators
        Value operator+(const Value& other) const;
        Value operator-(const Value& other) const;
        Value operator*(const Value& other) const;
        Value operator/(const Value& other) const;
        Value operator%(const Value& other) const;
        Value operator^(const Value& other) const;  // Exponentiation
        Value operator-() const;

        // Bitwise operators (missing)
        Value operator&(const Value& other) const;
        Value operator|(const Value& other) const;
        Value operator~() const;
        Value operator<<(const Value& other) const;
        Value operator>>(const Value& other) const;

        // String concatenation
        [[nodiscard]] Value concat(const Value& other) const;

        // Length operator
        [[nodiscard]] Value length() const;

        // Table access
        [[nodiscard]] Value get(const Value& key) const;
        void set(const Value& key, const Value& value);

        // Function call
        template <typename... Args>
        [[nodiscard]] Result<std::vector<Value>> call(Args&&... args) const;

        // Utility methods
        [[nodiscard]] String type_name() const;
        [[nodiscard]] String debug_string() const;
        [[nodiscard]] Size hash() const noexcept;

        // Raw access (use with caution)
        [[nodiscard]] const ValueVariant& raw() const noexcept { return data_; }
        ValueVariant& raw() noexcept { return data_; }

    private:
        ValueVariant data_;

        // Helper methods for operations
        static Result<Number> coerce_to_number(const Value& value) noexcept;
        static Result<String> coerce_to_string(const Value& value);
        static bool are_comparable(const Value& a, const Value& b) noexcept;
    };

    /**
     * @brief Value factory functions
     */
    namespace value_factory {

        inline Value nil() noexcept {
            return Value{};
        }
        inline Value boolean(bool b) noexcept {
            return Value(b);
        }
        inline Value number(Number n) noexcept {
            return Value(n);
        }
        inline Value integer(Int i) noexcept {
            return Value(i);
        }
        inline Value string(String s) {
            return Value(std::move(s));
        }
        inline Value string(StringView s) {
            return Value(s);
        }
        inline Value string(const char* s) {
            return Value(s);
        }

        Value table();
        Value table(std::initializer_list<std::pair<Value, Value>> init);
        Value function(std::function<std::vector<Value>(const std::vector<Value>&)> fn);
        Value userdata(void* ptr, String type_name = "userdata");
        Value thread();

    }  // namespace value_factory

    /**
     * @brief Value visitor for type-safe operations
     */
    template <typename Visitor>
    auto visit_value(Visitor&& visitor, const Value& value) {
        return std::visit(std::forward<Visitor>(visitor), value.raw());
    }

    /**
     * @brief Value range utilities for C++20 ranges
     */
    namespace value_ranges {

        /**
         * @brief Range view for table values
         */
        class TableView {
        public:
            explicit TableView(const Value& table) : table_(&table) {}

            class iterator {
            public:
                using value_type = std::pair<Value, Value>;
                using difference_type = std::ptrdiff_t;
                using pointer = const value_type*;
                using reference = const value_type&;
                using iterator_category = std::forward_iterator_tag;

                iterator() = default;
                explicit iterator(const Table& table, bool at_end = false);

                reference operator*() const;
                pointer operator->() const;
                iterator& operator++();
                iterator operator++(int);
                bool operator==(const iterator& other) const;
                bool operator!=(const iterator& other) const;

            private:
                const Table* table_ = nullptr;
                Size index_ = 0;
                mutable std::pair<Value, Value> current_;
                bool at_end_ = true;
            };

            [[nodiscard]] iterator begin() const;
            [[nodiscard]] iterator end() const;

        private:
            const Value* table_;
        };

        /**
         * @brief Create table view for range-based iteration
         */
        inline TableView make_table_view(const Value& table) {
            return TableView(table);
        }

    }  // namespace value_ranges

    /**
     * @brief Value type traits and concepts
     */
    template <typename T>
    concept ConvertibleToValue = requires(T t) { Value(t); };

    template <typename T>
    concept ConvertibleFromValue = requires(Value v) {
        { v.template to<T>() } -> std::same_as<Result<T>>;
    };

    template <typename T>
    struct ValueTraits {
        static constexpr bool is_value_type = false;
        static constexpr ValueType value_type = ValueType::Nil;
    };

    template <>
    struct ValueTraits<Value::Nil> {
        static constexpr bool is_value_type = true;
        static constexpr ValueType value_type = ValueType::Nil;
    };

    template <>
    struct ValueTraits<Value::Boolean> {
        static constexpr bool is_value_type = true;
        static constexpr ValueType value_type = ValueType::Boolean;
    };

    template <>
    struct ValueTraits<Value::Number> {
        static constexpr bool is_value_type = true;
        static constexpr ValueType value_type = ValueType::Number;
    };

    template <>
    struct ValueTraits<Value::String> {
        static constexpr bool is_value_type = true;
        static constexpr ValueType value_type = ValueType::String;
    };

    /**
     * @brief Value comparison utilities
     */
    namespace value_comparison {

        bool raw_equal(const Value& a, const Value& b) noexcept;
        bool lua_equal(const Value& a, const Value& b) noexcept;
        std::strong_ordering compare(const Value& a, const Value& b);

    }  // namespace value_comparison

}  // namespace rangelua::runtime

// Hash support for std::unordered_map
namespace std {
    template <>
    struct hash<rangelua::runtime::Value> {
        std::size_t operator()(const rangelua::runtime::Value& value) const noexcept {
            return value.hash();
        }
    };
}  // namespace std

// Concept verification - now enabled with improved concepts
#include "../core/concepts.hpp"
static_assert(rangelua::concepts::LuaValue<rangelua::runtime::Value>);
static_assert(rangelua::concepts::Hashable<rangelua::runtime::Value>);
static_assert(rangelua::concepts::Comparable<rangelua::runtime::Value>);
