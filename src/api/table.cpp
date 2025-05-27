/**
 * @file table.cpp
 * @brief Implementation of comprehensive Table API for RangeLua
 * @version 0.1.0
 */

#include <rangelua/api/table.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/utils/logger.hpp>

#include <stdexcept>

namespace rangelua::api {

    namespace {
        auto& logger() {
            static auto log = utils::Logger::create_logger("api.table");
            return log;
        }
    }  // namespace

    // Construction and conversion
    Table::Table() : table_(runtime::makeGCObject<runtime::Table>()) {
        logger()->debug("Created new empty table");
    }

    Table::Table(runtime::Value value) {
        if (value.is_table()) {
            auto result = value.to_table();
            if (std::holds_alternative<runtime::Value::TablePtr>(result)) {
                table_ = std::get<runtime::Value::TablePtr>(result);
                logger()->debug("Created table wrapper from Value");
            } else {
                logger()->error("Failed to extract table from Value");
                throw std::invalid_argument("Failed to extract table from Value");
            }
        } else {
            logger()->error("Attempted to create Table from non-table Value");
            throw std::invalid_argument("Value is not a table");
        }
    }

    Table::Table(runtime::GCPtr<runtime::Table> table) : table_(std::move(table)) {
        if (!table_) {
            logger()->error("Attempted to create Table from null GCPtr");
            throw std::invalid_argument("Table pointer is null");
        }
        logger()->debug("Created table wrapper from GCPtr");
    }

    Table::Table(std::initializer_list<std::pair<runtime::Value, runtime::Value>> init)
        : table_(runtime::makeGCObject<runtime::Table>()) {
        logger()->debug("Creating table with {} initial elements", init.size());
        for (const auto& [key, value] : init) {
            table_->set(key, value);
        }
    }

    // Validation
    bool Table::is_valid() const noexcept {
        return table_.get() != nullptr;
    }

    bool Table::is_table() const noexcept {
        return is_valid();
    }

    // Basic table operations
    runtime::Value Table::get(const runtime::Value& key) const {
        ensure_valid();
        return table_->get(key);
    }

    void Table::set(const runtime::Value& key, const runtime::Value& value) {
        ensure_valid();
        table_->set(key, value);
    }

    bool Table::has(const runtime::Value& key) const {
        ensure_valid();
        return table_->has(key);
    }

    void Table::remove(const runtime::Value& key) {
        ensure_valid();
        table_->remove(key);
    }

    // String key convenience methods
    runtime::Value Table::get(const String& key) const {
        return get(make_key(key));
    }

    void Table::set(const String& key, const runtime::Value& value) {
        set(make_key(key), value);
    }

    bool Table::has(const String& key) const {
        return has(make_key(key));
    }

    void Table::remove(const String& key) {
        remove(make_key(key));
    }

    // Array operations
    runtime::Value Table::get_array(Size index) const {
        ensure_valid();
        return table_->getArray(index);
    }

    void Table::set_array(Size index, const runtime::Value& value) {
        ensure_valid();
        table_->setArray(index, value);
    }

    Size Table::array_size() const noexcept {
        if (!is_valid()) return 0;
        return table_->arraySize();
    }

    // Size and capacity
    Size Table::size() const noexcept {
        if (!is_valid()) return 0;
        return table_->totalSize();
    }

    Size Table::hash_size() const noexcept {
        if (!is_valid()) return 0;
        return table_->hashSize();
    }

    Size Table::total_size() const noexcept {
        return size();
    }

    bool Table::empty() const noexcept {
        return size() == 0;
    }

    // Iterator implementation
    Table::Iterator::Iterator(const runtime::Table& table, bool end)
        : table_(&table), runtime_iter_(end ? table.end() : table.begin()), is_end_(end) {
        if (!is_end_ && runtime_iter_ != table_->end()) {
            current_pair_ = *runtime_iter_;
        }
    }

    Table::Iterator::reference Table::Iterator::operator*() const {
        if (is_end_) {
            throw std::runtime_error("Dereferencing end iterator");
        }
        return current_pair_;
    }

    Table::Iterator::pointer Table::Iterator::operator->() const {
        return &operator*();
    }

    Table::Iterator& Table::Iterator::operator++() {
        if (!is_end_) {
            ++runtime_iter_;
            if (runtime_iter_ != table_->end()) {
                current_pair_ = *runtime_iter_;
            } else {
                is_end_ = true;
            }
        }
        return *this;
    }

    Table::Iterator Table::Iterator::operator++(int) {
        Iterator temp = *this;
        ++(*this);
        return temp;
    }

    bool Table::Iterator::operator==(const Iterator& other) const {
        return table_ == other.table_ &&
               is_end_ == other.is_end_ &&
               (is_end_ || runtime_iter_ == other.runtime_iter_);
    }

    bool Table::Iterator::operator!=(const Iterator& other) const {
        return !(*this == other);
    }

    Table::Iterator Table::begin() const {
        ensure_valid();
        return Iterator(*table_, false);
    }

    Table::Iterator Table::end() const {
        ensure_valid();
        return Iterator(*table_, true);
    }

    // Metatable operations
    std::optional<Table> Table::get_metatable() const {
        ensure_valid();
        auto mt = table_->metatable();
        if (mt) {
            return Table(mt);
        }
        return std::nullopt;
    }

    void Table::set_metatable(const Table& metatable) {
        ensure_valid();
        metatable.ensure_valid();
        table_->setMetatable(metatable.table_);
    }

    void Table::set_metatable(std::nullptr_t) {
        ensure_valid();
        table_->setMetatable(runtime::GCPtr<runtime::Table>{});
    }

    bool Table::has_metatable() const noexcept {
        if (!is_valid()) return false;
        return table_->metatable().get() != nullptr;
    }

    // Conversion and access
    runtime::Value Table::to_value() const {
        ensure_valid();
        return runtime::Value(table_);
    }

    runtime::GCPtr<runtime::Table> Table::get_table() const {
        return table_;
    }

    const runtime::Table& Table::operator*() const {
        ensure_valid();
        return *table_;
    }

    const runtime::Table* Table::operator->() const {
        ensure_valid();
        return table_.get();
    }

    // Utility methods
    void Table::clear() {
        ensure_valid();
        // Clear both array and hash parts by creating a new table
        table_ = runtime::makeGCObject<runtime::Table>();
    }

    std::vector<runtime::Value> Table::keys() const {
        ensure_valid();
        std::vector<runtime::Value> result;
        for (const auto& [key, value] : *this) {
            result.push_back(key);
        }
        return result;
    }

    std::vector<runtime::Value> Table::values() const {
        ensure_valid();
        std::vector<runtime::Value> result;
        for (const auto& [key, value] : *this) {
            result.push_back(value);
        }
        return result;
    }

    // Comparison
    bool Table::operator==(const Table& other) const {
        return table_ == other.table_;
    }

    bool Table::operator!=(const Table& other) const {
        return !(*this == other);
    }

    // Private methods
    void Table::ensure_valid() const {
        if (!is_valid()) {
            logger()->error("Operation on invalid table");
            throw std::runtime_error("Table is not valid");
        }
    }

    runtime::Value Table::make_key(const String& key) const {
        return runtime::Value(key);
    }

    // Template method implementations
    template<>
    std::optional<String> Table::get_as<String>(const runtime::Value& key) const {
        auto value = get(key);
        if (value.is_string()) {
            auto result = value.to_string();
            if (std::holds_alternative<String>(result)) {
                return std::get<String>(result);
            }
        }
        return std::nullopt;
    }

    template<>
    std::optional<Number> Table::get_as<Number>(const runtime::Value& key) const {
        auto value = get(key);
        if (value.is_number()) {
            auto result = value.to_number();
            if (std::holds_alternative<Number>(result)) {
                return std::get<Number>(result);
            }
        }
        return std::nullopt;
    }

    template<>
    std::optional<Int> Table::get_as<Int>(const runtime::Value& key) const {
        auto value = get(key);
        if (value.is_number()) {
            auto result = value.to_number();
            if (std::holds_alternative<Number>(result)) {
                return static_cast<Int>(std::get<Number>(result));
            }
        }
        return std::nullopt;
    }

    template<>
    std::optional<bool> Table::get_as<bool>(const runtime::Value& key) const {
        auto value = get(key);
        if (value.is_boolean()) {
            auto result = value.to_boolean();
            if (std::holds_alternative<bool>(result)) {
                return std::get<bool>(result);
            }
        }
        return std::nullopt;
    }

    template<>
    void Table::set_value<String>(const runtime::Value& key, String&& value) {
        set(key, runtime::Value(std::forward<String>(value)));
    }

    template<>
    void Table::set_value<Number>(const runtime::Value& key, Number&& value) {
        set(key, runtime::Value(std::forward<Number>(value)));
    }

    template<>
    void Table::set_value<Int>(const runtime::Value& key, Int&& value) {
        set(key, runtime::Value(static_cast<Number>(std::forward<Int>(value))));
    }

    template<>
    void Table::set_value<bool>(const runtime::Value& key, bool&& value) {
        set(key, runtime::Value(std::forward<bool>(value)));
    }

    // String key versions
    template<>
    std::optional<String> Table::get_as<String>(const String& key) const {
        return get_as<String>(make_key(key));
    }

    template<>
    std::optional<Number> Table::get_as<Number>(const String& key) const {
        return get_as<Number>(make_key(key));
    }

    template<>
    std::optional<Int> Table::get_as<Int>(const String& key) const {
        return get_as<Int>(make_key(key));
    }

    template<>
    std::optional<bool> Table::get_as<bool>(const String& key) const {
        return get_as<bool>(make_key(key));
    }

    template<>
    void Table::set_value<String>(const String& key, String&& value) {
        set_value<String>(make_key(key), std::forward<String>(value));
    }

    template<>
    void Table::set_value<Number>(const String& key, Number&& value) {
        set_value<Number>(make_key(key), std::forward<Number>(value));
    }

    template<>
    void Table::set_value<Int>(const String& key, Int&& value) {
        set_value<Int>(make_key(key), std::forward<Int>(value));
    }

    template<>
    void Table::set_value<bool>(const String& key, bool&& value) {
        set_value<bool>(make_key(key), std::forward<bool>(value));
    }

    // Array versions
    template<>
    std::optional<String> Table::get_array_as<String>(Size index) const {
        auto value = get_array(index);
        if (value.is_string()) {
            auto result = value.to_string();
            if (std::holds_alternative<String>(result)) {
                return std::get<String>(result);
            }
        }
        return std::nullopt;
    }

    template<>
    std::optional<Number> Table::get_array_as<Number>(Size index) const {
        auto value = get_array(index);
        if (value.is_number()) {
            auto result = value.to_number();
            if (std::holds_alternative<Number>(result)) {
                return std::get<Number>(result);
            }
        }
        return std::nullopt;
    }

    template<>
    std::optional<Int> Table::get_array_as<Int>(Size index) const {
        auto value = get_array(index);
        if (value.is_number()) {
            auto result = value.to_number();
            if (std::holds_alternative<Number>(result)) {
                return static_cast<Int>(std::get<Number>(result));
            }
        }
        return std::nullopt;
    }

    template<>
    std::optional<bool> Table::get_array_as<bool>(Size index) const {
        auto value = get_array(index);
        if (value.is_boolean()) {
            auto result = value.to_boolean();
            if (std::holds_alternative<bool>(result)) {
                return std::get<bool>(result);
            }
        }
        return std::nullopt;
    }

    template<>
    void Table::set_array_value<String>(Size index, String&& value) {
        set_array(index, runtime::Value(std::forward<String>(value)));
    }

    template<>
    void Table::set_array_value<Number>(Size index, Number&& value) {
        set_array(index, runtime::Value(std::forward<Number>(value)));
    }

    template<>
    void Table::set_array_value<Int>(Size index, Int&& value) {
        set_array(index, runtime::Value(static_cast<Number>(std::forward<Int>(value))));
    }

    template<>
    void Table::set_array_value<bool>(Size index, bool&& value) {
        set_array(index, runtime::Value(std::forward<bool>(value)));
    }

}  // namespace rangelua::api
