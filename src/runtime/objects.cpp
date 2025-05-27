/**
 * @file objects.cpp
 * @brief Implementation of runtime objects (Table, Function, Userdata, Coroutine)
 * @version 0.1.0
 */

#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/objects.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rangelua::runtime {

    // Table implementation
    Table::Table() : GCObject(LuaType::TABLE) {
        // Constructor implementation moved here to avoid incomplete type issues
    }

    Table::~Table() = default;
    void Table::set(const Value& key, const Value& value) {
        if (isArrayIndex(key)) {
            auto num_result = key.to_number();
            if (std::holds_alternative<Number>(num_result)) {
                auto index = static_cast<Size>(std::get<Number>(num_result));
                setArray(index, value);
            }
        } else {
            hashPart_[key] = value;
        }
    }

    Value Table::get(const Value& key) const {
        if (isArrayIndex(key)) {
            auto num_result = key.to_number();
            if (std::holds_alternative<Number>(num_result)) {
                auto index = static_cast<Size>(std::get<Number>(num_result));
                return getArray(index);
            }
        }
        auto it = hashPart_.find(key);
        return (it != hashPart_.end()) ? it->second : Value{};
    }

    bool Table::has(const Value& key) const {
        if (isArrayIndex(key)) {
            auto num_result = key.to_number();
            if (std::holds_alternative<Number>(num_result)) {
                auto index = static_cast<Size>(std::get<Number>(num_result));
                return index > 0 && index <= arrayPart_.size();
            }
        }
        return hashPart_.find(key) != hashPart_.end();
    }

    void Table::remove(const Value& key) {
        if (isArrayIndex(key)) {
            auto num_result = key.to_number();
            if (std::holds_alternative<Number>(num_result)) {
                auto index = static_cast<Size>(std::get<Number>(num_result));
                if (index > 0 && index <= arrayPart_.size()) {
                    arrayPart_[index - 1] = Value{};  // Set to nil
                }
            }
        } else {
            hashPart_.erase(key);
        }
    }

    void Table::setArray(Size index, const Value& value) {
        if (index == 0) return;  // Lua arrays are 1-indexed

        if (index > arrayPart_.size()) {
            arrayPart_.resize(index);
        }
        arrayPart_[index - 1] = value;
    }

    Value Table::getArray(Size index) const {
        if (index == 0 || index > arrayPart_.size()) {
            return Value{};  // Return nil for out-of-bounds
        }
        return arrayPart_[index - 1];
    }

    Size Table::arraySize() const noexcept {
        return arrayPart_.size();
    }

    Size Table::hashSize() const noexcept {
        return hashPart_.size();
    }

    Size Table::totalSize() const noexcept {
        return arrayPart_.size() + hashPart_.size();
    }

    void Table::setMetatable(GCPtr<Table> metatable) {
        metatable_ = metatable;
    }

    GCPtr<Table> Table::metatable() {
        return metatable_;
    }

    void Table::traverse(std::function<void(GCObject*)> visitor) {
        // Traverse array part
        for (const auto& value : arrayPart_) {
            if (value.is_gc_object()) {
                visitor(value.as_gc_object());
            }
        }

        // Traverse hash part
        for (const auto& [key, value] : hashPart_) {
            if (key.is_gc_object()) {
                visitor(key.as_gc_object());
            }
            if (value.is_gc_object()) {
                visitor(value.as_gc_object());
            }
        }

        // Traverse metatable
        if (metatable_) {
            visitor(metatable_.get());
        }
    }

    Size Table::objectSize() const noexcept {
        return sizeof(Table) +
               arrayPart_.capacity() * sizeof(Value) +
               hashPart_.size() * (sizeof(Value) * 2);  // Approximate hash table overhead
    }

    void Table::optimizeStorage() {
        // Implementation for storage optimization
        // This could involve rehashing or resizing based on usage patterns
    }

    bool Table::isArrayIndex(const Value& key) const {
        if (!key.is_number()) return false;

        auto num_result = key.to_number();
        if (!std::holds_alternative<Number>(num_result)) return false;

        Number num = std::get<Number>(num_result);
        if (num <= 0 || num != std::floor(num)) return false;  // Must be positive integer

        return static_cast<Size>(num) <= arrayPart_.size() + 1;  // Allow one past end for growth
    }

    // Table::Iterator implementation
    Table::Iterator::Iterator(const Table& table, bool atEnd)
        : table_(table), atEnd_(atEnd) {
        if (!atEnd_) {
            // Start with array part
            if (table_.arrayPart_.empty()) {
                // Move to hash part
                inHashPart_ = true;
                hashIter_ = table_.hashPart_.begin();
                if (hashIter_ == table_.hashPart_.end()) {
                    atEnd_ = true;
                }
            }
        }
    }

    std::pair<Value, Value> Table::Iterator::operator*() const {
        if (atEnd_) {
            throw std::runtime_error("Iterator at end");
        }

        if (!inHashPart_) {
            // Array part
            return {Value(static_cast<Number>(arrayIndex_ + 1)), table_.arrayPart_[arrayIndex_]};
        } else {
            // Hash part
            return {hashIter_->first, hashIter_->second};
        }
    }

    Table::Iterator& Table::Iterator::operator++() {
        if (atEnd_) return *this;

        if (!inHashPart_) {
            // Array part
            ++arrayIndex_;
            if (arrayIndex_ >= table_.arrayPart_.size()) {
                // Move to hash part
                inHashPart_ = true;
                hashIter_ = table_.hashPart_.begin();
                if (hashIter_ == table_.hashPart_.end()) {
                    atEnd_ = true;
                }
            }
        } else {
            // Hash part
            ++hashIter_;
            if (hashIter_ == table_.hashPart_.end()) {
                atEnd_ = true;
            }
        }

        return *this;
    }

    bool Table::Iterator::operator==(const Iterator& other) const {
        if (atEnd_ && other.atEnd_) return true;
        if (atEnd_ != other.atEnd_) return false;

        if (inHashPart_ != other.inHashPart_) return false;

        if (!inHashPart_) {
            return arrayIndex_ == other.arrayIndex_;
        } else {
            return hashIter_ == other.hashIter_;
        }
    }

    bool Table::Iterator::operator!=(const Iterator& other) const {
        return !(*this == other);
    }

    // Function implementation
    Function::Function(CFunction func)
        : GCObject(LuaType::FUNCTION), type_(Type::C_FUNCTION), cFunction_(std::move(func)) {
    }

    Function::Function(std::vector<Instruction> bytecode, Size paramCount)
        : GCObject(LuaType::FUNCTION), type_(Type::LUA_FUNCTION),
          parameterCount_(paramCount), bytecode_(std::move(bytecode)) {
    }

    Function::Type Function::type() const noexcept {
        return type_;
    }

    Size Function::parameterCount() const noexcept {
        return parameterCount_;
    }

    Size Function::upvalueCount() const noexcept {
        return upvalues_.size();
    }

    bool Function::isCFunction() const noexcept {
        return type_ == Type::C_FUNCTION;
    }

    const Function::CFunction& Function::cFunction() const {
        if (!isCFunction()) {
            throw std::runtime_error("Function is not a C function");
        }
        return cFunction_;
    }

    bool Function::isLuaFunction() const noexcept {
        return type_ == Type::LUA_FUNCTION;
    }

    const std::vector<Instruction>& Function::bytecode() const {
        if (!isLuaFunction()) {
            throw std::runtime_error("Function is not a Lua function");
        }
        return bytecode_;
    }

    void Function::addUpvalue(const Value& value) {
        upvalues_.push_back(value);
        if (type_ != Type::CLOSURE) {
            type_ = Type::CLOSURE;
        }
    }

    Value Function::getUpvalue(Size index) const {
        if (index >= upvalues_.size()) {
            return Value{};  // Return nil for out-of-bounds
        }
        return upvalues_[index];
    }

    void Function::setUpvalue(Size index, const Value& value) {
        if (index >= upvalues_.size()) {
            upvalues_.resize(index + 1);
        }
        upvalues_[index] = value;
    }

    std::vector<Value> Function::call(const std::vector<Value>& args) const {
        if (isCFunction()) {
            return cFunction_(args);
        }
        // For Lua functions, this would need VM integration
        // For now, return empty result
        return {};
    }

    void Function::traverse(std::function<void(GCObject*)> visitor) {
        // Traverse upvalues
        for (const auto& upvalue : upvalues_) {
            if (upvalue.is_gc_object()) {
                visitor(upvalue.as_gc_object());
            }
        }

        // Traverse constants
        for (const auto& constant : constants_) {
            if (constant.is_gc_object()) {
                visitor(constant.as_gc_object());
            }
        }
    }

    Size Function::objectSize() const noexcept {
        return sizeof(Function) +
               bytecode_.capacity() * sizeof(Instruction) +
               constants_.capacity() * sizeof(Value) +
               upvalues_.capacity() * sizeof(Value);
    }

    // Userdata implementation
    Userdata::Userdata(void* data, Size size, String typeName)
        : GCObject(LuaType::USERDATA), data_(data), size_(size), typeName_(std::move(typeName)) {
    }

    void* Userdata::data() const noexcept {
        return data_;
    }

    Size Userdata::size() const noexcept {
        return size_;
    }

    const String& Userdata::typeName() const noexcept {
        return typeName_;
    }

    void Userdata::setMetatable(GCPtr<Table> metatable) {
        metatable_ = std::move(metatable);
    }

    GCPtr<Table> Userdata::metatable() const {
        return metatable_;
    }

    void Userdata::setUserValue(Size index, const Value& value) {
        if (index >= userValues_.size()) {
            userValues_.resize(index + 1);
        }
        userValues_[index] = value;
    }

    Value Userdata::getUserValue(Size index) const {
        if (index >= userValues_.size()) {
            return Value{};  // Return nil for out-of-bounds
        }
        return userValues_[index];
    }

    Size Userdata::userValueCount() const noexcept {
        return userValues_.size();
    }

    void Userdata::traverse(std::function<void(GCObject*)> visitor) {
        // Traverse metatable
        if (metatable_) {
            visitor(metatable_.get());
        }

        // Traverse user values
        for (const auto& userValue : userValues_) {
            if (userValue.is_gc_object()) {
                visitor(userValue.as_gc_object());
            }
        }
    }

    Size Userdata::objectSize() const noexcept {
        return sizeof(Userdata) +
               size_ +  // The actual user data
               userValues_.capacity() * sizeof(Value);
    }

    // Coroutine implementation
    Coroutine::Coroutine(Size stackSize)
        : GCObject(LuaType::THREAD), status_(Status::SUSPENDED) {
        stack_.reserve(stackSize);
    }

    Coroutine::Status Coroutine::status() const noexcept {
        return status_;
    }

    bool Coroutine::isResumable() const noexcept {
        return status_ == Status::SUSPENDED;
    }

    void Coroutine::push(const Value& value) {
        stack_.push_back(value);
    }

    Value Coroutine::pop() {
        if (stack_.empty()) {
            return Value{};  // Return nil for empty stack
        }
        Value value = stack_.back();
        stack_.pop_back();
        return value;
    }

    Value Coroutine::top() const {
        if (stack_.empty()) {
            return Value{};  // Return nil for empty stack
        }
        return stack_.back();
    }

    Size Coroutine::stackSize() const noexcept {
        return stack_.size();
    }

    bool Coroutine::stackEmpty() const noexcept {
        return stack_.empty();
    }

    std::vector<Value> Coroutine::resume() {
        return resume(std::vector<Value>{});
    }

    std::vector<Value> Coroutine::resume(const std::vector<Value>& args) {
        if (!isResumable()) {
            return {};  // Cannot resume
        }

        // Push arguments onto stack
        for (const auto& arg : args) {
            push(arg);
        }

        status_ = Status::RUNNING;

        // TODO: Implement actual coroutine execution
        // For now, just return the yielded values
        auto result = yieldedValues_;
        yieldedValues_.clear();

        status_ = Status::DEAD;  // Mark as finished for now
        return result;
    }

    std::vector<Value> Coroutine::yield() {
        return yield(std::vector<Value>{});
    }

    std::vector<Value> Coroutine::yield(const std::vector<Value>& values) {
        if (status_ != Status::RUNNING) {
            return {};  // Can only yield from running coroutine
        }

        yieldedValues_ = values;
        status_ = Status::SUSPENDED;

        return values;
    }

    void Coroutine::setError(const String& error) {
        error_ = error;
        status_ = Status::DEAD;
    }

    const String& Coroutine::error() const noexcept {
        return error_;
    }

    bool Coroutine::hasError() const noexcept {
        return !error_.empty();
    }

    void Coroutine::traverse(std::function<void(GCObject*)> visitor) {
        // Traverse stack
        for (const auto& value : stack_) {
            if (value.is_gc_object()) {
                visitor(value.as_gc_object());
            }
        }

        // Traverse yielded values
        for (const auto& value : yieldedValues_) {
            if (value.is_gc_object()) {
                visitor(value.as_gc_object());
            }
        }

        // Traverse current function
        if (currentFunction_) {
            visitor(currentFunction_.get());
        }
    }

    Size Coroutine::objectSize() const noexcept {
        return sizeof(Coroutine) +
               stack_.capacity() * sizeof(Value) +
               yieldedValues_.capacity() * sizeof(Value);
    }

}  // namespace rangelua::runtime
