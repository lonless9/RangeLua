#pragma once

/**
 * @file instruction.hpp
 * @brief Lua 5.5 compatible instruction definitions and utilities
 * @version 0.1.0
 */

#include <cstdint>

#include "types.hpp"

namespace rangelua {

    /**
     * @brief Lua 5.5 compatible opcodes (matching official implementation)
     */
    enum class OpCode : std::uint8_t {
        // Load operations
        OP_MOVE = 0,    // R[A] := R[B]
        OP_LOADI,       // R[A] := sBx (signed integer)
        OP_LOADF,       // R[A] := sBx (float)
        OP_LOADK,       // R[A] := K[Bx]
        OP_LOADKX,      // R[A] := K[extra arg]
        OP_LOADFALSE,   // R[A] := false
        OP_LFALSESKIP,  // R[A] := false; pc++
        OP_LOADTRUE,    // R[A] := true
        OP_LOADNIL,     // R[A], R[A+1], ..., R[A+B] := nil

        // Upvalue operations
        OP_GETUPVAL,  // R[A] := UpValue[B]
        OP_SETUPVAL,  // UpValue[B] := R[A]

        // Table operations
        OP_GETTABUP,  // R[A] := UpValue[B][K[C]:shortstring]
        OP_GETTABLE,  // R[A] := R[B][R[C]]
        OP_GETI,      // R[A] := R[B][C]
        OP_GETFIELD,  // R[A] := R[B][K[C]:shortstring]

        OP_SETTABUP,  // UpValue[A][K[B]:shortstring] := RK(C)
        OP_SETTABLE,  // R[A][R[B]] := RK(C)
        OP_SETI,      // R[A][B] := RK(C)
        OP_SETFIELD,  // R[A][K[B]:shortstring] := RK(C)

        OP_NEWTABLE,  // R[A] := {}
        OP_SELF,      // R[A+1] := R[B]; R[A] := R[B][K[C]:shortstring]

        // Arithmetic operations
        OP_ADDI,   // R[A] := R[B] + sC
        OP_ADDK,   // R[A] := R[B] + K[C]
        OP_SUBK,   // R[A] := R[B] - K[C]
        OP_MULK,   // R[A] := R[B] * K[C]
        OP_MODK,   // R[A] := R[B] % K[C]
        OP_POWK,   // R[A] := R[B] ^ K[C]
        OP_DIVK,   // R[A] := R[B] / K[C]
        OP_IDIVK,  // R[A] := R[B] // K[C]

        OP_BANDK,  // R[A] := R[B] & K[C]
        OP_BORK,   // R[A] := R[B] | K[C]
        OP_BXORK,  // R[A] := R[B] ~ K[C]

        OP_SHRI,  // R[A] := R[B] >> sC
        OP_SHLI,  // R[A] := R[B] << sC

        OP_ADD,   // R[A] := R[B] + R[C]
        OP_SUB,   // R[A] := R[B] - R[C]
        OP_MUL,   // R[A] := R[B] * R[C]
        OP_MOD,   // R[A] := R[B] % R[C]
        OP_POW,   // R[A] := R[B] ^ R[C]
        OP_DIV,   // R[A] := R[B] / R[C]
        OP_IDIV,  // R[A] := R[B] // R[C]

        OP_BAND,  // R[A] := R[B] & R[C]
        OP_BOR,   // R[A] := R[B] | R[C]
        OP_BXOR,  // R[A] := R[B] ~ R[C]
        OP_SHL,   // R[A] := R[B] << R[C]
        OP_SHR,   // R[A] := R[B] >> R[C]

        // Metamethod operations
        OP_MMBIN,   // call C metamethod over R[A] and R[B]
        OP_MMBINI,  // call C metamethod over R[A] and sB
        OP_MMBINK,  // call C metamethod over R[A] and K[B]

        // Unary operations
        OP_UNM,   // R[A] := -R[B]
        OP_BNOT,  // R[A] := ~R[B]
        OP_NOT,   // R[A] := not R[B]
        OP_LEN,   // R[A] := #R[B] (length operator)

        OP_CONCAT,  // R[A] := R[A].. ... ..R[A + B - 1]

        // Control flow
        OP_CLOSE,  // close all upvalues >= R[A]
        OP_TBC,    // mark variable A "to be closed"
        OP_JMP,    // pc += sJ

        // Comparison operations
        OP_EQ,  // if ((R[A] == R[B]) ~= k) then pc++
        OP_LT,  // if ((R[A] <  R[B]) ~= k) then pc++
        OP_LE,  // if ((R[A] <= R[B]) ~= k) then pc++

        OP_EQK,  // if ((R[A] == K[B]) ~= k) then pc++
        OP_EQI,  // if ((R[A] == sB) ~= k) then pc++
        OP_LTI,  // if ((R[A] < sB) ~= k) then pc++
        OP_LEI,  // if ((R[A] <= sB) ~= k) then pc++
        OP_GTI,  // if ((R[A] > sB) ~= k) then pc++
        OP_GEI,  // if ((R[A] >= sB) ~= k) then pc++

        OP_TEST,     // if (not R[A] == k) then pc++
        OP_TESTSET,  // if (not R[B] == k) then pc++ else R[A] := R[B]

        // Function operations
        OP_CALL,      // R[A], ... ,R[A+C-2] := R[A](R[A+1], ... ,R[A+B-1])
        OP_TAILCALL,  // return R[A](R[A+1], ... ,R[A+B-1])

        OP_RETURN,   // return R[A], ... ,R[A+B-2]
        OP_RETURN0,  // return
        OP_RETURN1,  // return R[A]

        // Loop operations
        OP_FORLOOP,  // update counters; if loop continues then pc-=Bx;
        OP_FORPREP,  // <check values and prepare counters>; if not to run then pc+=Bx+1;

        OP_TFORPREP,  // create upvalue for R[A + 3]; pc+=Bx
        OP_TFORCALL,  // R[A+4], ... ,R[A+3+C] := R[A](R[A+1], R[A+2]);
        OP_TFORLOOP,  // if R[A+2] ~= nil then { R[A]=R[A+2]; pc -= Bx }

        OP_SETLIST,  // R[A][vC+i] := R[A+i], 1 <= i <= vB

        OP_CLOSURE,  // R[A] := closure(KPROTO[Bx])

        OP_VARARG,      // R[A], R[A+1], ..., R[A+C-2] = vararg
        OP_VARARGPREP,  // (adjust vararg parameters)

        OP_EXTRAARG,  // extra (larger) argument for previous opcode

        // Total number of opcodes
        NUM_OPCODES
    };

    /**
     * @brief Enhanced Lua 5.5 compatible instruction format with C++20 features
     */
    struct LuaInstruction {
        // Lua 5.5 instruction format constants
        static constexpr Size OPCODE_BITS = 7;
        static constexpr Size A_BITS = 8;
        static constexpr Size B_BITS = 8;
        static constexpr Size C_BITS = 8;
        static constexpr Size BX_BITS = 17;
        static constexpr Size SBX_BITS = 17;
        static constexpr Size AX_BITS = 25;

        // Maximum values for each field
        static constexpr Size MAX_A = (1U << A_BITS) - 1U;
        static constexpr Size MAX_B = (1U << B_BITS) - 1U;
        static constexpr Size MAX_C = (1U << C_BITS) - 1U;
        static constexpr Size MAX_BX = (1U << BX_BITS) - 1U;
        static constexpr Size MAX_SBX = (1U << (SBX_BITS - 1U)) - 1U;
        static constexpr Size MAX_AX = (1U << AX_BITS) - 1U;

        std::uint32_t raw;

        explicit constexpr LuaInstruction(std::uint32_t instruction = 0) noexcept
            : raw(instruction) {}

        // Modern C++20 comparison operators
        constexpr auto operator<=>(const LuaInstruction&) const noexcept = default;

        // Field extraction methods
        [[nodiscard]] constexpr OpCode opcode() const noexcept {
            return static_cast<OpCode>(raw & ((1U << OPCODE_BITS) - 1U));
        }

        [[nodiscard]] constexpr std::uint8_t A() const noexcept {
            return static_cast<std::uint8_t>((raw >> OPCODE_BITS) & ((1U << A_BITS) - 1U));
        }

        [[nodiscard]] constexpr std::uint8_t B() const noexcept {
            return static_cast<std::uint8_t>((raw >> (OPCODE_BITS + A_BITS)) &
                                             ((1U << B_BITS) - 1U));
        }

        [[nodiscard]] constexpr std::uint8_t C() const noexcept {
            return static_cast<std::uint8_t>((raw >> (OPCODE_BITS + A_BITS + B_BITS)) &
                                             ((1U << C_BITS) - 1U));
        }

        [[nodiscard]] constexpr std::uint32_t Bx() const noexcept {
            return (raw >> (OPCODE_BITS + A_BITS)) & ((1U << BX_BITS) - 1U);
        }

        [[nodiscard]] constexpr std::int32_t sBx() const noexcept {
            constexpr std::uint32_t bias = (1U << (BX_BITS - 1U)) - 1U;
            const auto unsigned_result = Bx();
            return static_cast<std::int32_t>(unsigned_result) - static_cast<std::int32_t>(bias);
        }

        [[nodiscard]] constexpr std::uint32_t Ax() const noexcept {
            return (raw >> OPCODE_BITS) & ((1U << AX_BITS) - 1U);
        }

        // Instruction creation methods
        static constexpr LuaInstruction
        create_abc(OpCode op, std::uint8_t a, std::uint8_t b, std::uint8_t c) noexcept {
            return LuaInstruction(
                static_cast<std::uint32_t>(op) | (static_cast<std::uint32_t>(a) << OPCODE_BITS) |
                (static_cast<std::uint32_t>(b) << (OPCODE_BITS + A_BITS)) |
                (static_cast<std::uint32_t>(c) << (OPCODE_BITS + A_BITS + B_BITS)));
        }

        static constexpr LuaInstruction
        create_abx(OpCode op, std::uint8_t a, std::uint32_t bx) noexcept {
            return LuaInstruction(static_cast<std::uint32_t>(op) |
                                  (static_cast<std::uint32_t>(a) << OPCODE_BITS) |
                                  ((bx & ((1U << BX_BITS) - 1U)) << (OPCODE_BITS + A_BITS)));
        }

        static constexpr LuaInstruction
        create_asbx(OpCode op, std::uint8_t a, std::int32_t sbx) noexcept {
            constexpr std::uint32_t bias = (1U << (BX_BITS - 1U)) - 1U;
            const auto unsigned_sbx =
                static_cast<std::uint32_t>(sbx + static_cast<std::int32_t>(bias));
            return create_abx(op, a, unsigned_sbx);
        }

        static constexpr LuaInstruction create_ax(OpCode op, std::uint32_t ax) noexcept {
            return LuaInstruction(static_cast<std::uint32_t>(op) |
                                  ((ax & ((1U << AX_BITS) - 1U)) << OPCODE_BITS));
        }

        // Modern C++20 utility methods
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            const auto op = opcode();
            return static_cast<std::uint8_t>(op) < static_cast<std::uint8_t>(OpCode::NUM_OPCODES);
        }

        [[nodiscard]] constexpr bool uses_register_a() const noexcept {
            // Most instructions use register A
            return true;
        }

        [[nodiscard]] constexpr bool uses_register_b() const noexcept {
            const auto op = opcode();
            // Instructions that don't use B register
            return op != OpCode::OP_LOADNIL && op != OpCode::OP_LOADFALSE &&
                   op != OpCode::OP_LOADTRUE && op != OpCode::OP_RETURN0;
        }

        [[nodiscard]] constexpr bool uses_register_c() const noexcept {
            const auto op = opcode();
            // Only ABC format instructions use C register
            return op == OpCode::OP_ADD || op == OpCode::OP_SUB || op == OpCode::OP_MUL ||
                   op == OpCode::OP_DIV || op == OpCode::OP_MOD || op == OpCode::OP_POW ||
                   op == OpCode::OP_BAND || op == OpCode::OP_BOR || op == OpCode::OP_BXOR ||
                   op == OpCode::OP_SHL || op == OpCode::OP_SHR || op == OpCode::OP_GETTABLE ||
                   op == OpCode::OP_SETTABLE || op == OpCode::OP_NEWTABLE;
        }

        [[nodiscard]] constexpr bool is_jump_instruction() const noexcept {
            const auto op = opcode();
            return op == OpCode::OP_JMP || op == OpCode::OP_FORLOOP || op == OpCode::OP_FORPREP ||
                   op == OpCode::OP_TFORLOOP || op == OpCode::OP_EQ || op == OpCode::OP_LT ||
                   op == OpCode::OP_LE || op == OpCode::OP_TEST || op == OpCode::OP_TESTSET;
        }

        [[nodiscard]] constexpr bool modifies_register_a() const noexcept {
            const auto op = opcode();
            // Instructions that don't modify register A
            return op != OpCode::OP_SETUPVAL && op != OpCode::OP_SETTABUP &&
                   op != OpCode::OP_SETTABLE && op != OpCode::OP_SETI &&
                   op != OpCode::OP_SETFIELD && op != OpCode::OP_JMP;
        }
    };

    // Modern C++20 instruction utility functions
    namespace instruction_utils {

        /**
         * @brief Get instruction format as string for debugging
         */
        [[nodiscard]] constexpr StringView get_instruction_format(OpCode op) noexcept {
            switch (op) {
                case OpCode::OP_LOADK:
                case OpCode::OP_LOADKX:
                case OpCode::OP_FORLOOP:
                case OpCode::OP_FORPREP:
                case OpCode::OP_TFORPREP:
                case OpCode::OP_CLOSURE:
                    return "ABx";

                case OpCode::OP_LOADI:
                case OpCode::OP_LOADF:
                case OpCode::OP_JMP:
                    return "AsBx";

                case OpCode::OP_EXTRAARG:
                    return "Ax";

                default:
                    return "ABC";
            }
        }

        /**
         * @brief Check if instruction is safe (doesn't cause side effects)
         */
        [[nodiscard]] constexpr bool is_safe_instruction(OpCode op) noexcept {
            switch (op) {
                case OpCode::OP_MOVE:
                case OpCode::OP_LOADI:
                case OpCode::OP_LOADF:
                case OpCode::OP_LOADK:
                case OpCode::OP_LOADKX:
                case OpCode::OP_LOADFALSE:
                case OpCode::OP_LOADTRUE:
                case OpCode::OP_LOADNIL:
                case OpCode::OP_UNM:
                case OpCode::OP_NOT:
                case OpCode::OP_LEN:
                case OpCode::OP_BNOT:
                case OpCode::OP_ADD:
                case OpCode::OP_SUB:
                case OpCode::OP_MUL:
                case OpCode::OP_DIV:
                case OpCode::OP_MOD:
                case OpCode::OP_POW:
                case OpCode::OP_BAND:
                case OpCode::OP_BOR:
                case OpCode::OP_BXOR:
                case OpCode::OP_SHL:
                case OpCode::OP_SHR:
                    return true;
                default:
                    return false;
            }
        }

    }  // namespace instruction_utils

}  // namespace rangelua
