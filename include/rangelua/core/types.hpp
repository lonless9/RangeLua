#pragma once

/**
 * @file types.hpp
 * @brief Core type definitions and aliases for RangeLua
 * @version 0.1.0
 */

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace rangelua {

// Basic integer types
using Int = std::int64_t;
using UInt = std::uint64_t;
using Size = std::size_t;
using Byte = std::uint8_t;

// Floating point types
using Number = double;
using Float = float;

// String types
using String = std::string;
using StringView = std::string_view;

// Memory types
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
using Span = std::span<T>;

// Optional and variant types
template<typename T>
using Optional = std::optional<T>;

template<typename... Ts>
using Variant = std::variant<Ts...>;

// Instruction and bytecode types
using Instruction = std::uint32_t;
using Register = std::uint8_t;
using Constant = std::uint16_t;
using Jump = std::int16_t;

// VM execution types
using StackIndex = std::int32_t;
using UpvalueIndex = std::uint8_t;
using LocalIndex = std::uint8_t;

// Source location information
struct SourceLocation {
    String filename_;
    Size line_ = 0;
    Size column_ = 0;

    constexpr SourceLocation() noexcept = default;
    constexpr SourceLocation(String source_file, Size source_line, Size source_column) noexcept
        : filename_(std::move(source_file)), line_(source_line), column_(source_column) {}
};

// Forward declarations for core types
class Value;
class Table;
class Function;
class Coroutine;
class State;
class VM;

// Memory management forward declarations
class GarbageCollector;
class MemoryManager;

// Frontend forward declarations
class Lexer;
class Parser;
class AST;

// Backend forward declarations
class CodeGenerator;
class Optimizer;
class BytecodeEmitter;

// Lua value type tags (matching Lua 5.5)
enum class LuaType : std::uint8_t {
    NIL = 0,
    BOOLEAN = 1,
    LIGHTUSERDATA = 2,
    NUMBER = 3,
    STRING = 4,
    TABLE = 5,
    FUNCTION = 6,
    USERDATA = 7,
    THREAD = 8,
    // Internal types
    UPVALUE = 9,
    PROTO = 10,
    DEADKEY = 11
};

// Error handling types
enum class ErrorCode : std::uint8_t {
    SUCCESS = 0,
    SYNTAX_ERROR,
    RUNTIME_ERROR,
    MEMORY_ERROR,
    TYPE_ERROR,
    ARGUMENT_ERROR,
    STACK_OVERFLOW,
    COROUTINE_ERROR,
    IO_ERROR,
    UNKNOWN_ERROR
};

// Result type for operations that can fail
template<typename T>
using Result = std::variant<T, ErrorCode>;

// Success/failure result for operations without return value
using Status = Result<std::monostate>;

// Utility type traits
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept StringLike = std::convertible_to<T, StringView>;

template<typename T>
concept Callable = std::invocable<T>;

// Range concepts for modern C++20 usage
template<typename R>
concept Range = std::ranges::range<R>;

template<typename R, typename T>
concept RangeOf = Range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

// GC object header (matching Lua 5.5 CommonHeader)
struct GCHeader {
    struct GCObject* next = nullptr;
    LuaType type;
    std::uint8_t marked = 0;

    explicit constexpr GCHeader(LuaType t) noexcept
        : type(t) {}
};

// Tagged value system (inspired by Lua 5.5 TValue)
template<typename T>
struct TaggedValue {
    T value;
    LuaType tag;

    constexpr TaggedValue() noexcept : value{}, tag(LuaType::NIL) {}
    constexpr TaggedValue(T v, LuaType t) noexcept : value(v), tag(t) {}

    [[nodiscard]] constexpr bool is_type(LuaType t) const noexcept { return tag == t; }
    [[nodiscard]] constexpr LuaType type() const noexcept { return tag; }
};

// Forward declarations for bytecode components (defined in backend/bytecode.hpp)
// OpCode enum class is defined in backend/bytecode.hpp

} // namespace rangelua
