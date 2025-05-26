#pragma once

/**
 * @file bytecode.hpp
 * @brief Bytecode instruction definitions and emission
 * @version 0.1.0
 */

#include <span>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/instruction.hpp"
#include "../core/types.hpp"

namespace rangelua::backend {

    // OpCode enum is now defined in core/instruction.hpp

    /**
     * @brief Constant value type for bytecode constants
     *
     * Represents all possible Lua constant types that can be stored in bytecode:
     * - nil (std::monostate)
     * - boolean (bool)
     * - number (double)
     * - integer (Int)
     * - string (String)
     */
    using ConstantValue = Variant<std::monostate, bool, Number, Int, String>;

    /**
     * @brief Constant value type enumeration
     */
    enum class ConstantType : std::uint8_t { Nil = 0, Boolean, Number, Integer, String };

    /**
     * @brief Get the type of a constant value
     */
    constexpr ConstantType get_constant_type(const ConstantValue& value) noexcept {
        return static_cast<ConstantType>(value.index());
    }

    /**
     * @brief Check if constant value is nil
     */
    constexpr bool is_nil_constant(const ConstantValue& value) noexcept {
        return std::holds_alternative<std::monostate>(value);
    }

    /**
     * @brief Check if constant value is boolean
     */
    constexpr bool is_boolean_constant(const ConstantValue& value) noexcept {
        return std::holds_alternative<bool>(value);
    }

    /**
     * @brief Check if constant value is number
     */
    constexpr bool is_number_constant(const ConstantValue& value) noexcept {
        return std::holds_alternative<Number>(value);
    }

    /**
     * @brief Check if constant value is integer
     */
    constexpr bool is_integer_constant(const ConstantValue& value) noexcept {
        return std::holds_alternative<Int>(value);
    }

    /**
     * @brief Check if constant value is string
     */
    constexpr bool is_string_constant(const ConstantValue& value) noexcept {
        return std::holds_alternative<String>(value);
    }

    /**
     * @brief Convert frontend LiteralExpression::Value to ConstantValue
     */
    ConstantValue
    to_constant_value(const Variant<Number, Int, String, bool, std::monostate>& literal_value);

    /**
     * @brief Convert ConstantValue to string representation for debugging
     */
    String constant_value_to_string(const ConstantValue& value);

    /**
     * @brief Check if two constant values are equal
     */
    bool constant_values_equal(const ConstantValue& a, const ConstantValue& b) noexcept;

    /**
     * @brief Hash function for ConstantValue (for use in unordered containers)
     */
    struct ConstantValueHash {
        Size operator()(const ConstantValue& value) const noexcept;
    };

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
        static Register decode_a(Instruction instr) noexcept { return LuaInstruction(instr).A(); }

        /**
         * @brief Decode B field
         */
        static Register decode_b(Instruction instr) noexcept { return LuaInstruction(instr).B(); }

        /**
         * @brief Decode C field
         */
        static Register decode_c(Instruction instr) noexcept { return LuaInstruction(instr).C(); }

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
        std::vector<ConstantValue> constants;
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
         * @brief Add constant value to constant pool
         * @return Constant index
         */
        Size add_constant(const ConstantValue& value);

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
        std::unordered_map<String, Size> string_constant_map_;  // For string constants only
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
     *
     * Thread-safe validator that returns validation errors directly without global state.
     * Uses modern C++20 patterns and RAII principles for clean error handling.
     */
    class BytecodeValidator {
    public:
        /**
         * @brief Validation error information
         */
        struct ValidationError {
            Size instruction_index = 0;
            String message;
            ErrorCode code = ErrorCode::SUCCESS;

            ValidationError() noexcept = default;
            ValidationError(Size index, String msg, ErrorCode error_code) noexcept
                : instruction_index(index), message(std::move(msg)), code(error_code) {}

            // Equality comparison for testing
            bool operator==(const ValidationError& other) const noexcept {
                return instruction_index == other.instruction_index && message == other.message &&
                       code == other.code;
            }
        };

        /**
         * @brief Result type for validation operations
         *
         * Returns either an empty vector (success) or a vector of validation errors.
         * Thread-safe and does not rely on global state.
         */
        using ValidationResult = Result<std::vector<ValidationError>>;

        /**
         * @brief Validate bytecode function
         *
         * @param function The bytecode function to validate
         * @return ValidationResult containing validation errors (empty vector if valid)
         *
         * This method is thread-safe and does not modify any global state.
         * Each call starts with a clean validation context.
         */
        static ValidationResult validate(const BytecodeFunction& function) noexcept;

        /**
         * @brief Validate single instruction
         *
         * @param instr The instruction to validate
         * @param function The containing function for context
         * @param index The instruction index within the function
         * @return Result containing ValidationError if invalid, success otherwise
         *
         * This method validates a single instruction in isolation and is thread-safe.
         */
        static Result<std::monostate> validate_instruction(Instruction instr,
                                                           const BytecodeFunction& function,
                                                           Size index) noexcept;

        /**
         * @brief Check if validation result indicates success
         *
         * @param result The validation result to check
         * @return true if validation succeeded (no errors), false otherwise
         */
        static bool is_valid(const ValidationResult& result) noexcept {
            return is_success(result) && get_value(result).empty();
        }

        /**
         * @brief Get validation errors from result
         *
         * @param result The validation result
         * @return Vector of validation errors (empty if validation succeeded)
         */
        static const std::vector<ValidationError>&
        get_errors(const ValidationResult& result) noexcept {
            static const std::vector<ValidationError> empty_errors;
            return is_success(result) ? get_value(result) : empty_errors;
        }

    private:
        // No static state - all methods are stateless and thread-safe

        /**
         * @brief Validate instruction format and operands
         */
        static Result<std::monostate> validate_instruction_format(Instruction instr,
                                                                  Size index) noexcept;

        /**
         * @brief Validate register references
         */
        static Result<std::monostate> validate_register_usage(Instruction instr,
                                                              const BytecodeFunction& function,
                                                              Size index) noexcept;

        /**
         * @brief Validate constant references
         */
        static Result<std::monostate> validate_constant_usage(Instruction instr,
                                                              const BytecodeFunction& function,
                                                              Size index) noexcept;

        /**
         * @brief Validate jump targets
         */
        static Result<std::monostate> validate_jump_targets(Instruction instr,
                                                            const BytecodeFunction& function,
                                                            Size index) noexcept;
    };

}  // namespace rangelua::backend

// Concept verification (commented out until concepts are properly included)
// static_assert(rangelua::concepts::BytecodeEmitter<rangelua::backend::BytecodeEmitter>);
