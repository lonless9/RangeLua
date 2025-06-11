/**
 * @file objects.cpp
 * @brief Implementation of runtime objects (Table, Function, Userdata, Coroutine)
 * @version 0.1.0
 */

#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>

#include <cmath>
#include <stdexcept>

namespace rangelua::runtime {

    // Table implementation
    Table::Table() : GCObject(LuaType::TABLE) {
        // Constructor implementation moved here to avoid incomplete type issues
    }

    Table::~Table() {
        // Explicitly clear containers to break potential circular references
        hashPart_.clear();
        arrayPart_.clear();
        metatable_.reset();
    }
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

    Size Table::rawLength() const noexcept {
        // Calculate Lua-style length: consecutive non-nil elements from index 1
        Size length = 0;
        for (Size i = 0; i < arrayPart_.size(); ++i) {
            if (arrayPart_[i].is_nil()) {
                break;  // Stop at first nil
            }
            length = i + 1;  // Convert to 1-based index
        }
        return length;
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

    GCPtr<Table> Table::metatable() const {
        return metatable_;
    }

    void Table::traverse(AdvancedGarbageCollector& gc) {
        // Traverse array part
        for (const auto& value : arrayPart_) {
            if (value.is_gc_object()) {
                gc.markObject(value.as_gc_object());
            }
        }

        // Traverse hash part
        for (const auto& [key, value] : hashPart_) {
            if (key.is_gc_object()) {
                gc.markObject(key.as_gc_object());
            }
            if (value.is_gc_object()) {
                gc.markObject(value.as_gc_object());
            }
        }

        // Traverse metatable
        if (metatable_) {
            gc.markObject(metatable_.get());
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

        // Allow any positive integer as array index - table will grow as needed
        return true;
    }

    // Table::Iterator implementation
    Table::Iterator::Iterator(const Table& table, bool atEnd)
        : table_(table), arrayIndex_(0), inHashPart_(false), atEnd_(atEnd) {
        if (!atEnd_) {
            // Start with array part - find first non-nil element
            while (arrayIndex_ < table_.arrayPart_.size() &&
                   table_.arrayPart_[arrayIndex_].is_nil()) {
                ++arrayIndex_;
            }

            if (arrayIndex_ >= table_.arrayPart_.size()) {
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
            // Array part - find next non-nil element
            ++arrayIndex_;
            while (arrayIndex_ < table_.arrayPart_.size() &&
                   table_.arrayPart_[arrayIndex_].is_nil()) {
                ++arrayIndex_;
            }

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

    // Upvalue implementation
    Upvalue::Upvalue(Value* stack_location)
        : GCObject(LuaType::UPVALUE), stackLocation_(stack_location), isOpen_(true) {}

    Upvalue::Upvalue(Value value)
        : GCObject(LuaType::UPVALUE), closedValue_(std::move(value)), isOpen_(false) {}

    Upvalue::~Upvalue() {
        if (isOpen_) {
            // For open upvalues, we don't own the stack location
            stackLocation_ = nullptr;
        } else {
            // For closed upvalues, explicitly destroy the value
            closedValue_.~Value();
        }
    }

    bool Upvalue::isOpen() const noexcept {
        return isOpen_;
    }

    bool Upvalue::isClosed() const noexcept {
        return !isOpen_;
    }

    const Value& Upvalue::getValue() const noexcept {
        if (isOpen_) {
            static Value nil_value;  // Static nil value for when stackLocation_ is null
            return stackLocation_ ? *stackLocation_ : nil_value;
        }
        return closedValue_;
    }

    void Upvalue::setValue(const Value& value) {
        if (isOpen_) {
            if (stackLocation_) {
                *stackLocation_ = value;
            }
        } else {
            closedValue_ = value;
        }
    }

    void Upvalue::close() {
        if (isOpen_ && stackLocation_) {
            // Copy the stack value to local storage
            Value stack_value = *stackLocation_;
            stackLocation_ = nullptr;
            isOpen_ = false;

            // Use placement new to initialize the closed value
            new (&closedValue_) Value(std::move(stack_value));
        }
    }

    Value* Upvalue::getStackLocation() const noexcept {
        return isOpen_ ? stackLocation_ : nullptr;
    }

    void Upvalue::setStackLocation(Value* location) noexcept {
        if (isOpen_) {
            stackLocation_ = location;
        }
    }

    void Upvalue::traverse(AdvancedGarbageCollector& gc) {
        // Visit the value if it's closed and contains GC objects
        if (!isOpen_) {
            if (closedValue_.is_gc_object()) {
                gc.markObject(closedValue_.as_gc_object());
            }
        }
    }

    Size Upvalue::objectSize() const noexcept {
        return sizeof(Upvalue);
    }

    // Function implementation
    Function::Function(CFunction func)
        : GCObject(LuaType::FUNCTION), type_(Type::C_FUNCTION), cFunction_(std::move(func)) {
    }

    Function::Function(std::vector<Instruction> bytecode, Size paramCount)
        : GCObject(LuaType::FUNCTION), type_(Type::LUA_FUNCTION),
          parameterCount_(paramCount), bytecode_(std::move(bytecode)) {
    }

    Function::~Function() {
        // Explicitly clear containers to break potential circular references
        upvalues_.clear();
        constants_.clear();
        bytecode_.clear();
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

    bool Function::isVararg() const noexcept {
        return isVararg_;
    }

    void Function::setVararg(bool vararg) noexcept {
        isVararg_ = vararg;
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
        if (!isLuaFunction() && !isClosure()) {
            throw std::runtime_error("Function is not a Lua function or closure");
        }
        return bytecode_;
    }

    void Function::addConstant(const Value& constant) {
        constants_.push_back(constant);
    }

    const std::vector<Value>& Function::constants() const {
        return constants_;
    }

    bool Function::isClosure() const noexcept {
        return type_ == Type::CLOSURE;
    }

    void Function::makeClosure() {
        if (type_ != Type::CLOSURE) {
            type_ = Type::CLOSURE;
        }
    }

    void Function::addUpvalue(GCPtr<Upvalue> upvalue) {
        upvalues_.push_back(std::move(upvalue));
        if (type_ != Type::CLOSURE) {
            type_ = Type::CLOSURE;
        }
    }

    GCPtr<Upvalue> Function::getUpvalue(Size index) const {
        if (index >= upvalues_.size()) {
            return GCPtr<Upvalue>{};  // Return empty GCPtr for out-of-bounds
        }
        return upvalues_[index];
    }

    void Function::setUpvalue(Size index, GCPtr<Upvalue> upvalue) {
        if (index >= upvalues_.size()) {
            upvalues_.resize(index + 1);
        }
        upvalues_[index] = std::move(upvalue);
    }

    // Legacy upvalue interface (for compatibility)
    void Function::addUpvalue(const Value& value) {
        // Create a closed upvalue with the given value
        auto upvalue = GCPtr<Upvalue>(new Upvalue(value));
        addUpvalue(upvalue);
    }

    Value Function::getUpvalueValue(Size index) const {
        auto upvalue = getUpvalue(index);
        return upvalue ? upvalue->getValue() : Value{};
    }

    void Function::setUpvalueValue(Size index, const Value& value) {
        auto upvalue = getUpvalue(index);
        if (upvalue) {
            upvalue->setValue(value);
        } else {
            // Create new closed upvalue if it doesn't exist
            auto new_upvalue = GCPtr<Upvalue>(new Upvalue(value));
            setUpvalue(index, new_upvalue);
        }
    }

    std::vector<Value> Function::call(const std::vector<Value>& args) const {
        if (isCFunction()) {
            return cFunction_(args);
        }

        // For Lua functions, we need VM integration
        // This is a placeholder that should be called through the VM
        // The actual execution should happen in the VM's call mechanism
        throw std::runtime_error("Lua function calls must be executed through the VM");
    }

    void Function::traverse(AdvancedGarbageCollector& gc) {
        // Traverse upvalues
        for (const auto& upvalue : upvalues_) {
            if (upvalue) {
                gc.markObject(upvalue.get());
            }
        }

        // Traverse constants if they contain GC objects
        for (const auto& constant : constants_) {
            if (constant.is_gc_object()) {
                gc.markObject(constant.as_gc_object());
            }
        }
    }

    Size Function::objectSize() const noexcept {
        return sizeof(Function) + bytecode_.capacity() * sizeof(Instruction) +
               constants_.capacity() * sizeof(Value) +
               upvalues_.capacity() * sizeof(GCPtr<Upvalue>);
    }

    // Userdata implementation
    Userdata::Userdata(void* data, Size size, String typeName)
        : GCObject(LuaType::USERDATA), data_(data), size_(size), typeName_(std::move(typeName)) {
    }

    Userdata::~Userdata() {
        // Explicitly clear containers to break potential circular references
        userValues_.clear();
        metatable_.reset();
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

    void Userdata::traverse(AdvancedGarbageCollector& gc) {
        // Traverse metatable
        if (metatable_) {
            gc.markObject(metatable_.get());
        }

        // Traverse user values
        for (const auto& userValue : userValues_) {
            if (userValue.is_gc_object()) {
                gc.markObject(userValue.as_gc_object());
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

    Coroutine::~Coroutine() {
        // Explicitly clear containers to break potential circular references
        stack_.clear();
        yieldedValues_.clear();
        currentFunction_.reset();
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

    void Coroutine::traverse(AdvancedGarbageCollector& gc) {
        // Traverse stack
        for (const auto& value : stack_) {
            if (value.is_gc_object()) {
                gc.markObject(value.as_gc_object());
            }
        }

        // Traverse yielded values
        for (const auto& value : yieldedValues_) {
            if (value.is_gc_object()) {
                gc.markObject(value.as_gc_object());
            }
        }

        // Traverse current function
        if (currentFunction_) {
            gc.markObject(currentFunction_.get());
        }
    }

    Size Coroutine::objectSize() const noexcept {
        return sizeof(Coroutine) +
               stack_.capacity() * sizeof(Value) +
               yieldedValues_.capacity() * sizeof(Value);
    }

}  // namespace rangelua::runtime
