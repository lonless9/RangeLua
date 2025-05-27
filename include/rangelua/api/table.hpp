#pragma once

/**
 * @file table.hpp
 * @brief Comprehensive Table API for RangeLua
 * @version 0.1.0
 *
 * Provides a high-level C++ interface for Lua tables with RAII semantics,
 * type safety, and integration with the runtime objects system.
 */

#include <initializer_list>
#include <optional>
#include <vector>

#include "../core/types.hpp"
#include "../runtime/objects.hpp"
#include "../runtime/value.hpp"

namespace rangelua::api {

    /**
     * @brief High-level Table wrapper with comprehensive API
     *
     * Provides a safe, convenient interface for working with Lua tables.
     * Automatically manages the underlying runtime::Table object and provides
     * type-safe operations with proper error handling.
     */
    class Table {
    public:
        // Construction and conversion
        explicit Table();
        explicit Table(runtime::Value value);
        explicit Table(runtime::GCPtr<runtime::Table> table);
        Table(std::initializer_list<std::pair<runtime::Value, runtime::Value>> init);

        // Copy and move semantics
        Table(const Table& other) = default;
        Table& operator=(const Table& other) = default;
        Table(Table&& other) noexcept = default;
        Table& operator=(Table&& other) noexcept = default;

        ~Table() = default;

        // Validation
        [[nodiscard]] bool is_valid() const noexcept;
        [[nodiscard]] bool is_table() const noexcept;

        // Basic table operations
        [[nodiscard]] runtime::Value get(const runtime::Value& key) const;
        void set(const runtime::Value& key, const runtime::Value& value);
        [[nodiscard]] bool has(const runtime::Value& key) const;
        void remove(const runtime::Value& key);

        // Convenience accessors for common types
        template <typename T>
        [[nodiscard]] std::optional<T> get_as(const runtime::Value& key) const;

        template <typename T>
        void set_value(const runtime::Value& key, T&& value);

        // String key convenience methods
        [[nodiscard]] runtime::Value get(const String& key) const;
        void set(const String& key, const runtime::Value& value);
        [[nodiscard]] bool has(const String& key) const;
        void remove(const String& key);

        template <typename T>
        [[nodiscard]] std::optional<T> get_as(const String& key) const;

        template <typename T>
        void set_value(const String& key, T&& value);

        // Array operations
        [[nodiscard]] runtime::Value get_array(Size index) const;
        void set_array(Size index, const runtime::Value& value);
        [[nodiscard]] Size array_size() const noexcept;

        template <typename T>
        [[nodiscard]] std::optional<T> get_array_as(Size index) const;

        template <typename T>
        void set_array_value(Size index, T&& value);

        // Size and capacity
        [[nodiscard]] Size size() const noexcept;
        [[nodiscard]] Size hash_size() const noexcept;
        [[nodiscard]] Size total_size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        // Iteration support
        class Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<runtime::Value, runtime::Value>;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            explicit Iterator(const runtime::Table& table, bool end = false);

            reference operator*() const;
            pointer operator->() const;
            Iterator& operator++();
            Iterator operator++(int);
            bool operator==(const Iterator& other) const;
            bool operator!=(const Iterator& other) const;

        private:
            const runtime::Table* table_;
            runtime::Table::Iterator runtime_iter_;
            mutable value_type current_pair_;
            bool is_end_;
        };

        [[nodiscard]] Iterator begin() const;
        [[nodiscard]] Iterator end() const;

        // Metatable operations
        [[nodiscard]] std::optional<Table> get_metatable() const;
        void set_metatable(const Table& metatable);
        void set_metatable(std::nullptr_t);
        [[nodiscard]] bool has_metatable() const noexcept;

        // Conversion and access
        [[nodiscard]] runtime::Value to_value() const;
        [[nodiscard]] runtime::GCPtr<runtime::Table> get_table() const;
        [[nodiscard]] const runtime::Table& operator*() const;
        [[nodiscard]] const runtime::Table* operator->() const;

        // Utility methods
        void clear();
        [[nodiscard]] std::vector<runtime::Value> keys() const;
        [[nodiscard]] std::vector<runtime::Value> values() const;

        // Comparison
        bool operator==(const Table& other) const;
        bool operator!=(const Table& other) const;

    private:
        runtime::GCPtr<runtime::Table> table_;

        void ensure_valid() const;
        [[nodiscard]] runtime::Value make_key(const String& key) const;
    };

}  // namespace rangelua::api
