#pragma once

/**
 * @file metamethod.hpp
 * @brief Lua metamethod system implementation
 * @version 0.1.0
 */

#include <array>
#include <string_view>
#include <optional>

#include "../core/types.hpp"
#include "../core/error.hpp"
#include "value.hpp"

namespace rangelua::runtime {

    // Forward declarations
    class Table;
    class Value;

    /**
     * @brief Lua metamethod enumeration (matching Lua 5.5)
     *
     * Order is important and matches the official Lua implementation.
     * Fast-access metamethods (INDEX through EQ) are optimized.
     */
    enum class Metamethod : std::uint8_t {
        // Fast-access metamethods
        INDEX = 0,  // __index
        NEWINDEX,   // __newindex
        GC,         // __gc
        MODE,       // __mode
        LEN,        // __len
        EQ,         // __eq (last fast-access metamethod)

        // Arithmetic metamethods
        ADD,   // __add
        SUB,   // __sub
        MUL,   // __mul
        MOD,   // __mod
        POW,   // __pow
        DIV,   // __div
        IDIV,  // __idiv

        // Bitwise metamethods
        BAND,  // __band
        BOR,   // __bor
        BXOR,  // __bxor
        SHL,   // __shl
        SHR,   // __shr

        // Unary metamethods
        UNM,   // __unm (unary minus)
        BNOT,  // __bnot (bitwise not)

        // Comparison metamethods
        LT,  // __lt
        LE,  // __le

        // Other metamethods
        CONCAT,    // __concat
        CALL,      // __call
        TOSTRING,  // __tostring
        CLOSE,     // __close

        // Count of metamethods
        COUNT
    };

    /**
     * @brief Metamethod name constants
     */
    constexpr std::array<std::string_view, static_cast<size_t>(Metamethod::COUNT)>
        METAMETHOD_NAMES = {"__index",    "__newindex", "__gc",  "__mode", "__len",    "__eq",
                            "__add",      "__sub",      "__mul", "__mod",  "__pow",    "__div",
                            "__idiv",     "__band",     "__bor", "__bxor", "__shl",    "__shr",
                            "__unm",      "__bnot",     "__lt",  "__le",   "__concat", "__call",
                            "__tostring", "__close"};

    /**
     * @brief Fast-access metamethod mask
     *
     * Bit mask for fast-access metamethods (INDEX through EQ).
     * Used for optimization in metatable lookups.
     */
    constexpr std::uint32_t FAST_ACCESS_MASK = ~(~0u << (static_cast<int>(Metamethod::EQ) + 1));

    /**
     * @brief Metamethod lookup and invocation utilities
     */
    class MetamethodSystem {
    public:
        /**
         * @brief Get metamethod name as string
         * @param mm Metamethod enum value
         * @return String view of metamethod name
         */
        static constexpr std::string_view get_name(Metamethod mm) noexcept {
            auto index = static_cast<size_t>(mm);
            return index < METAMETHOD_NAMES.size() ? METAMETHOD_NAMES[index] : "__unknown";
        }

        /**
         * @brief Find metamethod by name
         * @param name Metamethod name (e.g., "__add")
         * @return Metamethod enum or nullopt if not found
         */
        static std::optional<Metamethod> find_by_name(std::string_view name) noexcept;

        /**
         * @brief Check if metamethod is fast-access
         * @param mm Metamethod to check
         * @return True if fast-access metamethod
         */
        static constexpr bool is_fast_access(Metamethod mm) noexcept {
            return static_cast<int>(mm) <= static_cast<int>(Metamethod::EQ);
        }

        /**
         * @brief Get metamethod from table's metatable
         * @param table Table to search
         * @param mm Metamethod to find
         * @return Metamethod value or nil if not found
         */
        static Value get_metamethod(const Table* table, Metamethod mm);

        /**
         * @brief Get metamethod from value's metatable
         * @param value Value to search
         * @param mm Metamethod to find
         * @return Metamethod value or nil if not found
         */
        static Value get_metamethod(const Value& value, Metamethod mm);

        /**
         * @brief Check if metamethod exists and is not nil
         * @param table Table to check
         * @param mm Metamethod to check
         * @return True if metamethod exists and is not nil
         */
        static bool has_metamethod(const Table* table, Metamethod mm);

        /**
         * @brief Check if value has metamethod
         * @param value Value to check
         * @param mm Metamethod to check
         * @return True if metamethod exists and is not nil
         */
        static bool has_metamethod(const Value& value, Metamethod mm);

        /**
         * @brief Call metamethod with arguments
         * @param metamethod Metamethod function to call
         * @param args Arguments to pass
         * @return Result of metamethod call
         */
        static Result<std::vector<Value>> call_metamethod(const Value& metamethod,
                                                          const std::vector<Value>& args);

        /**
         * @brief Call metamethod with arguments using VM context
         * @param context VM context for calling Lua functions
         * @param metamethod Metamethod function to call
         * @param args Arguments to pass
         * @return Result of metamethod call
         */
        static Result<std::vector<Value>> call_metamethod(class IVMContext& context,
                                                          const Value& metamethod,
                                                          const std::vector<Value>& args);

        /**
         * @brief Try binary operation with metamethod fallback
         * @param left Left operand
         * @param right Right operand
         * @param mm Metamethod to try
         * @return Result value or error
         */
        static Result<Value> try_binary_metamethod(const Value& left, const Value& right, Metamethod mm);

        /**
         * @brief Try binary operation with metamethod fallback using VM context
         * @param context VM context for calling Lua functions
         * @param left Left operand
         * @param right Right operand
         * @param mm Metamethod to try
         * @return Result value or error
         */
        static Result<Value> try_binary_metamethod(class IVMContext& context,
                                                   const Value& left,
                                                   const Value& right,
                                                   Metamethod mm);

        /**
         * @brief Try unary operation with metamethod fallback
         * @param operand Operand
         * @param mm Metamethod to try
         * @return Result value or error
         */
        static Result<Value> try_unary_metamethod(const Value& operand, Metamethod mm);

        /**
         * @brief Try unary operation with metamethod fallback using VM context
         * @param context VM context for calling Lua functions
         * @param operand Operand
         * @param mm Metamethod to try
         * @return Result value or error
         */
        static Result<Value>
        try_unary_metamethod(class IVMContext& context, const Value& operand, Metamethod mm);

        /**
         * @brief Try comparison with metamethod fallback
         * @param left Left operand
         * @param right Right operand
         * @param mm Metamethod to try
         * @return Comparison result or error
         */
        static Result<bool> try_comparison_metamethod(const Value& left, const Value& right, Metamethod mm);

        /**
         * @brief Try comparison with metamethod fallback using VM context
         * @param context VM context for calling Lua functions
         * @param left Left operand
         * @param right Right operand
         * @param mm Metamethod to try
         * @return Comparison result or error
         */
        static Result<bool> try_comparison_metamethod(class IVMContext& context,
                                                      const Value& left,
                                                      const Value& right,
                                                      Metamethod mm);
    };

    /**
     * @brief Metamethod operation context for VM instructions
     */
    struct MetamethodContext {
        Metamethod method;
        Value left_operand;
        Value right_operand;
        bool has_right_operand = true;

        // For immediate and constant operations
        std::optional<Number> immediate_value;
        std::optional<Size> constant_index;
    };

}  // namespace rangelua::runtime
