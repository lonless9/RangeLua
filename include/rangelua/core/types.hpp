#pragma once

/**
 * @file types.hpp
 * @brief Core type definitions and aliases for RangeLua with modern C++20 features
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

    // Basic integer types with concepts
    using Int = std::int64_t;
    using UInt = std::uint64_t;
    using Size = std::size_t;
    using Byte = std::uint8_t;
    using lu_byte = Byte;

    // Bitwise operation macros
#define bitmask(b) (1 << (b))
#define l_setbit(x, b) ((x) |= bitmask(b))
#define testbit(x, b) ((x) & bitmask(b))
#define resetbit(x, b) ((x) &= ~bitmask(b))

    // Floating point types
    using Number = double;
    using Float = float;

    // String types with concepts
    using String = std::string;
    using StringView = std::string_view;

    // Modern C++20 memory types with RAII guarantees
    template <typename T>
    using UniquePtr = std::unique_ptr<T>;

    template <typename T>
    using SharedPtr = std::shared_ptr<T>;

    template <typename T>
    using WeakPtr = std::weak_ptr<T>;

    template <typename T>
    using Span = std::span<T>;

    // Optional and variant types with enhanced type safety
    template <typename T>
    using Optional = std::optional<T>;

    template <typename... Ts>
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
    template <typename T>
    using Result = std::variant<T, ErrorCode>;

    // Success/failure result for operations without return value
    using Status = Result<std::monostate>;

    // Enhanced C++20 type concepts with better constraints
    template <typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    template <typename T>
    concept StringLike =
        std::convertible_to<T, StringView> || std::same_as<std::decay_t<T>, String> ||
        std::same_as<std::decay_t<T>, StringView>;

    template <typename T>
    concept Callable = std::invocable<T>;

    template <typename T, typename... Args>
    concept CallableWith = std::invocable<T, Args...>;

    // Enhanced range concepts for modern C++20 usage
    template <typename R>
    concept Range = std::ranges::range<R>;

    template <typename R, typename T>
    concept RangeOf = Range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

    template <typename R>
    concept ContiguousRange = std::ranges::contiguous_range<R>;

    template <typename R>
    concept SizedRange = std::ranges::sized_range<R>;

    // Modern C++20 memory management concepts
    template <typename T>
    concept SmartPointer = requires(T t) {
        typename T::element_type;
        { t.get() } -> std::convertible_to<typename T::element_type*>;
        { t.reset() } -> std::same_as<void>;
        { static_cast<bool>(t) } -> std::same_as<bool>;
    };

    template <typename T>
    concept UniquePointerLike = SmartPointer<T> && requires(T t) {
        { t.release() } -> std::convertible_to<typename T::element_type*>;
    };

    template <typename T>
    concept SharedPointerLike = SmartPointer<T> && requires(T t) {
        { t.use_count() } -> std::convertible_to<Int>;
    };

    // Modern C++20 RAII resource management concepts
    template <typename T>
    concept RAIIResource = requires(T t) {
        typename T::resource_type;
        { t.get() } -> std::convertible_to<typename T::resource_type>;
        { t.release() } -> std::same_as<void>;
    };

    // Enhanced GC object header with modern C++20 patterns
    struct GCHeader;

    // Enhanced tagged value system with C++20 features
    template <typename T>
    struct TaggedValue {
        T value;
        LuaType tag;

        constexpr TaggedValue() noexcept : value{}, tag(LuaType::NIL) {}
        constexpr TaggedValue(T v, LuaType t) noexcept : value(v), tag(t) {}

        [[nodiscard]] constexpr bool is_type(LuaType t) const noexcept { return tag == t; }
        [[nodiscard]] constexpr LuaType type() const noexcept { return tag; }

        // Modern C++20 comparison operators
        constexpr auto operator<=>(const TaggedValue&) const noexcept = default;

        // Type-safe value access with concepts
        template <typename U>
            requires std::convertible_to<T, U>
        [[nodiscard]] constexpr U as() const noexcept {
            return static_cast<U>(value);
        }
    };

    // Forward declarations for bytecode components (defined in backend/bytecode.hpp)
    // OpCode enum class is defined in backend/bytecode.hpp

}  // namespace rangelua
