#pragma once

/**
 * @file bytecode.hpp
 * @brief Bytecode instruction definitions and emission
 * @version 0.1.0
 */

#include <span>
#include <unordered_map>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/instruction.hpp"
#include "../core/types.hpp"

namespace rangelua::backend {

// OpCode enum is now defined in core/instruction.hpp

/**
 * @brief Instruction encoder/decoder using LuaInstruction from core/instruction.hpp
 */
class InstructionEncoder {
public:
    // Use constants from LuaInstruction
    static constexpr Size OPCODE_BITS = LuaInstruction::OPCODE_BITS;
    static constexpr Size A_BITS = LuaInstruction::A_BITS;
    static constexpr Size B_BITS = LuaInstruction::B_BITS;
    static constexpr Size C_BITS = LuaInstruction::C_BITS;
    static constexpr Size BX_BITS = LuaInstruction::BX_BITS;
    static constexpr Size SBX_BITS = LuaInstruction::SBX_BITS;
    static constexpr Size AX_BITS = LuaInstruction::AX_BITS;

    static constexpr Size MAX_A = LuaInstruction::MAX_A;
    static constexpr Size MAX_B = LuaInstruction::MAX_B;
    static constexpr Size MAX_C = LuaInstruction::MAX_C;
    static constexpr Size MAX_BX = LuaInstruction::MAX_BX;
    static constexpr Size MAX_SBX = LuaInstruction::MAX_SBX;
    static constexpr Size MAX_AX = LuaInstruction::MAX_AX;

    /**
     * @brief Encode instruction with A, B, C format
     */
    static Instruction encode_abc(OpCode op, Register a, Register b, Register c) noexcept {
        return LuaInstruction::create_abc(op, a, b, c).raw;
    }

    /**
     * @brief Encode instruction with A, Bx format
     */
    static Instruction encode_abx(OpCode op, Register a, std::uint32_t bx) noexcept {
        return LuaInstruction::create_abx(op, a, bx).raw;
    }

    /**
     * @brief Encode instruction with A, sBx format
     */
    static Instruction encode_asbx(OpCode op, Register a, std::int32_t sbx) noexcept {
        return LuaInstruction::create_asbx(op, a, sbx).raw;
    }

    /**
     * @brief Encode instruction with Ax format
     */
    static Instruction encode_ax(OpCode op, std::uint32_t ax) noexcept {
        return LuaInstruction::create_ax(op, ax).raw;
    }

    /**
     * @brief Decode instruction opcode
     */
    static OpCode decode_opcode(Instruction instr) noexcept {
        return LuaInstruction(instr).opcode();
    }

    /**
     * @brief Decode A field
     */
    static Register decode_a(Instruction instr) noexcept {
        return LuaInstruction(instr).A();
    }

    /**
     * @brief Decode B field
     */
    static Register decode_b(Instruction instr) noexcept {
        return LuaInstruction(instr).B();
    }

    /**
     * @brief Decode C field
     */
    static Register decode_c(Instruction instr) noexcept {
        return LuaInstruction(instr).C();
    }

    /**
     * @brief Decode Bx field
     */
    static std::uint32_t decode_bx(Instruction instr) noexcept {
        return LuaInstruction(instr).Bx();
    }

    /**
     * @brief Decode sBx field
     */
    static std::int32_t decode_sbx(Instruction instr) noexcept {
        return LuaInstruction(instr).sBx();
    }

    /**
     * @brief Decode Ax field
     */
    static std::uint32_t decode_ax(Instruction instr) noexcept {
        return LuaInstruction(instr).Ax();
    }
};

/**
 * @brief Bytecode function representation
 */
struct BytecodeFunction {
    String name;
    std::vector<Instruction> instructions;
    std::vector<String> constants;
    std::vector<String> locals;
    std::vector<String> upvalues;
    Size parameter_count = 0;
    Size stack_size = 0;
    bool is_vararg = false;

    // Debug information
    std::vector<Size> line_info;
    String source_name;
};

/**
 * @brief Bytecode emitter for generating instructions
 */
class BytecodeEmitter {
public:
    explicit BytecodeEmitter(String function_name = "<main>");

    ~BytecodeEmitter() = default;

    // Non-copyable, movable
    BytecodeEmitter(const BytecodeEmitter&) = delete;
    BytecodeEmitter& operator=(const BytecodeEmitter&) = delete;
    BytecodeEmitter(BytecodeEmitter&&) noexcept = default;
    BytecodeEmitter& operator=(BytecodeEmitter&&) noexcept = default;

    /**
     * @brief Emit instruction with A, B, C format
     * @return Instruction index
     */
    Size emit_abc(OpCode op, Register a, Register b, Register c);

    /**
     * @brief Emit instruction with A, Bx format
     * @return Instruction index
     */
    Size emit_abx(OpCode op, Register a, std::uint32_t bx);

    /**
     * @brief Emit instruction with A, sBx format
     * @return Instruction index
     */
    Size emit_asbx(OpCode op, Register a, std::int32_t sbx);

    /**
     * @brief Emit instruction with Ax format
     * @return Instruction index
     */
    Size emit_ax(OpCode op, std::uint32_t ax);

    /**
     * @brief Emit raw instruction
     * @return Instruction index
     */
    Size emit_instruction(Instruction instr);

    /**
     * @brief Patch instruction at given index
     */
    void patch_instruction(Size index, Instruction instr);

    /**
     * @brief Get current instruction count
     */
    [[nodiscard]] Size instruction_count() const noexcept;

    /**
     * @brief Add constant to constant pool
     * @return Constant index
     */
    Size add_constant(const String& value);

    /**
     * @brief Add local variable
     * @return Local index
     */
    Size add_local(const String& name);

    /**
     * @brief Add upvalue
     * @return Upvalue index
     */
    Size add_upvalue(const String& name);

    /**
     * @brief Set function parameters
     */
    void set_parameter_count(Size count);

    /**
     * @brief Set stack size requirement
     */
    void set_stack_size(Size size);

    /**
     * @brief Set vararg flag
     */
    void set_vararg(bool is_vararg);

    /**
     * @brief Add line information for debugging
     */
    void add_line_info(Size line);

    /**
     * @brief Set source name for debugging
     */
    void set_source_name(String name);

    /**
     * @brief Get generated bytecode function
     */
    [[nodiscard]] BytecodeFunction get_function() const;

    /**
     * @brief Get instruction span
     */
    [[nodiscard]] Span<const Instruction> instructions() const noexcept;

    /**
     * @brief Reset emitter state
     */
    void reset();

private:
    BytecodeFunction function_;
    std::unordered_map<String, Size> constant_map_;
    std::unordered_map<String, Size> local_map_;
    std::unordered_map<String, Size> upvalue_map_;
};

/**
 * @brief Bytecode disassembler for debugging
 */
class Disassembler {
public:
    /**
     * @brief Disassemble single instruction
     */
    static String disassemble_instruction(Instruction instr, Size index = 0);

    /**
     * @brief Disassemble bytecode function
     */
    static String disassemble_function(const BytecodeFunction& function);

    /**
     * @brief Get opcode name
     */
    static StringView opcode_name(OpCode op) noexcept;

    /**
     * @brief Get instruction format description
     */
    static StringView instruction_format(OpCode op) noexcept;
};

/**
 * @brief Bytecode validator for correctness checking
 */
class BytecodeValidator {
public:
    struct ValidationError {
        Size instruction_index;
        String message;
        ErrorCode code;
    };

    /**
     * @brief Validate bytecode function
     */
    static Result<std::monostate> validate(const BytecodeFunction& function);

    /**
     * @brief Validate single instruction
     */
    static Result<std::monostate> validate_instruction(Instruction instr, const BytecodeFunction& function, Size index);

    /**
     * @brief Get validation errors
     */
    static const std::vector<ValidationError>& errors() noexcept;

private:
    static std::vector<ValidationError> validation_errors_;
};

} // namespace rangelua::backend

// Concept verification (commented out until concepts are properly included)
// static_assert(rangelua::concepts::BytecodeEmitter<rangelua::backend::BytecodeEmitter>);
