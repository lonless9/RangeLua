#pragma once

/**
 * @file objects.hpp
 * @brief GC-managed object implementations for RangeLua
 * @version 0.1.0
 *
 * This file contains the concrete implementations of Lua objects
 * that are managed by the garbage collector:
 * - Table: Lua tables with array and hash parts
 * - Function: Lua functions (both C and Lua)
 * - Userdata: User-defined data with metatables
 * - Coroutine: Lua coroutines/threads
 */

#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>

#include "../core/types.hpp"
#include "gc.hpp"  // Include full GC system

namespace rangelua::runtime {

    // Forward declarations to avoid circular dependency
    class Value;

    /**
     * @brief Lua table implementation
     *
     * Implements Lua's hybrid array/hash table structure similar to Lua 5.5.
     * Uses separate array and hash parts for optimal performance.
     */
    class Table : public GCObject {
    public:
        explicit Table();
        ~Table() override;

        // Table operations
        void set(const Value& key, const Value& value);
        [[nodiscard]] Value get(const Value& key) const;
        [[nodiscard]] bool has(const Value& key) const;
        void remove(const Value& key);

        // Array operations
        void setArray(Size index, const Value& value);
        [[nodiscard]] Value getArray(Size index) const;
        [[nodiscard]] Size arraySize() const noexcept;

        // Hash operations
        [[nodiscard]] Size hashSize() const noexcept;
        [[nodiscard]] Size totalSize() const noexcept;

        // Metatable support
        void setMetatable(GCPtr<Table> metatable);
        [[nodiscard]] GCPtr<Table> metatable();

        // Iteration support
        class Iterator {
        public:
            explicit Iterator(const Table& table, bool atEnd = false);

            std::pair<Value, Value> operator*() const;
            Iterator& operator++();
            bool operator==(const Iterator& other) const;
            bool operator!=(const Iterator& other) const;

        private:
            const Table& table_;
            Size arrayIndex_ = 0;
            std::unordered_map<Value, Value>::const_iterator hashIter_;
            bool inHashPart_ = false;
            bool atEnd_ = false;
        };

        [[nodiscard]] Iterator begin() const { return Iterator(*this); }
        [[nodiscard]] Iterator end() const { return Iterator(*this, true); }

        // GCObject interface
        void traverse(std::function<void(GCObject*)> visitor) override;
        [[nodiscard]] Size objectSize() const noexcept override;

    private:
        std::vector<Value> arrayPart_;
        std::unordered_map<Value, Value> hashPart_;
        GCPtr<Table> metatable_;

        // Optimization: track if we need to resize
        [[maybe_unused]] mutable bool needsResize_ = false;
        void optimizeStorage();
        [[nodiscard]] bool isArrayIndex(const Value& key) const;
    };

    /**
     * @brief Lua function implementation
     *
     * Supports both C functions and Lua bytecode functions.
     * Includes upvalue management and closure support.
     */
    class Function : public GCObject {
    public:
        // Function types
        enum class Type : std::uint8_t {
            C_FUNCTION,      // C function
            LUA_FUNCTION,    // Lua bytecode function
            CLOSURE          // Function with upvalues
        };

        using CFunction = std::function<std::vector<Value>(const std::vector<Value>&)>;

        // Constructors
        explicit Function(CFunction func);

        explicit Function(std::vector<Instruction> bytecode, Size paramCount = 0);

        ~Function() override = default;

        // Function properties
        [[nodiscard]] Type type() const noexcept;
        [[nodiscard]] Size parameterCount() const noexcept;
        [[nodiscard]] Size upvalueCount() const noexcept;

        // C function access
        [[nodiscard]] bool isCFunction() const noexcept;
        [[nodiscard]] const CFunction& cFunction() const;

        // Lua function access
        [[nodiscard]] bool isLuaFunction() const noexcept;
        [[nodiscard]] const std::vector<Instruction>& bytecode() const;

        // Upvalue management
        void addUpvalue(const Value& value);
        [[nodiscard]] Value getUpvalue(Size index) const;
        void setUpvalue(Size index, const Value& value);

        // Call interface
        [[nodiscard]] std::vector<Value> call(const std::vector<Value>& args) const;

        // GCObject interface
        void traverse(std::function<void(GCObject*)> visitor) override;
        [[nodiscard]] Size objectSize() const noexcept override;

    private:
        Type type_;
        Size parameterCount_ = 0;

        // C function data
        CFunction cFunction_;

        // Lua function data
        std::vector<Instruction> bytecode_;
        std::vector<Value> constants_;

        // Upvalues (for closures)
        std::vector<Value> upvalues_;

        // Debug information
        String name_;
        String source_;
        [[maybe_unused]] Size lineNumber_ = 0;
    };

    /**
     * @brief Lua userdata implementation
     *
     * Wraps arbitrary C++ data with Lua metatable support.
     * Provides type-safe access to user-defined data.
     */
    class Userdata : public GCObject {
    public:
        explicit Userdata(void* data, Size size, String typeName = "userdata");

        ~Userdata() override = default;

        // Data access
        [[nodiscard]] void* data() const noexcept;
        [[nodiscard]] Size size() const noexcept;
        [[nodiscard]] const String& typeName() const noexcept;

        // Type-safe access
        template<typename T>
        [[nodiscard]] T* as() const noexcept;

        template<typename T>
        [[nodiscard]] bool is() const noexcept;

        // Metatable support
        void setMetatable(GCPtr<Table> metatable);
        [[nodiscard]] GCPtr<Table> metatable() const;

        // User values (additional Lua values associated with userdata)
        void setUserValue(Size index, const Value& value);
        [[nodiscard]] Value getUserValue(Size index) const;
        [[nodiscard]] Size userValueCount() const noexcept;

        // GCObject interface
        void traverse(std::function<void(GCObject*)> visitor) override;
        [[nodiscard]] Size objectSize() const noexcept override;

    private:
        void* data_;
        Size size_;
        String typeName_;
        GCPtr<Table> metatable_;
        std::vector<Value> userValues_;
    };

    /**
     * @brief Lua coroutine implementation
     *
     * Implements Lua coroutines with stack management and yield/resume support.
     * Provides cooperative multitasking within the Lua VM.
     */
    class Coroutine : public GCObject {
    public:
        enum class Status : std::uint8_t {
            SUSPENDED,   // Coroutine is suspended (can be resumed)
            RUNNING,     // Coroutine is currently running
            NORMAL,      // Coroutine is active but not running (calling another coroutine)
            DEAD         // Coroutine has finished or encountered an error
        };

        explicit Coroutine(Size stackSize = 1000);

        ~Coroutine() override = default;

        // Coroutine control
        [[nodiscard]] Status status() const noexcept;
        [[nodiscard]] bool isResumable() const noexcept;

        // Stack management
        void push(const Value& value);
        [[nodiscard]] Value pop();
        [[nodiscard]] Value top() const;
        [[nodiscard]] Size stackSize() const noexcept;
        [[nodiscard]] bool stackEmpty() const noexcept;

        // Execution control
        std::vector<Value> resume();
        std::vector<Value> resume(const std::vector<Value>& args);
        std::vector<Value> yield();
        std::vector<Value> yield(const std::vector<Value>& values);

        // Error handling
        void setError(const String& error);
        [[nodiscard]] const String& error() const noexcept;
        [[nodiscard]] bool hasError() const noexcept;

        // GCObject interface
        void traverse(std::function<void(GCObject*)> visitor) override;
        [[nodiscard]] Size objectSize() const noexcept override;

    private:
        Status status_;
        std::vector<Value> stack_;
        String error_;

        // Execution state
        [[maybe_unused]] Size programCounter_ = 0;
        GCPtr<Function> currentFunction_;

        // Yield/resume state
        std::vector<Value> yieldedValues_;
    };

}  // namespace rangelua::runtime
