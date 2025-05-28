/**
 * @file bytecode.cpp
 * @brief Bytecode implementation
 * @version 0.1.0
 */

#include <rangelua/backend/bytecode.hpp>

#include <iomanip>
#include <sstream>

namespace rangelua::backend {

    // Utility functions for ConstantValue
    ConstantValue
    to_constant_value(const Variant<Number, Int, String, bool, std::monostate>& literal_value) {
        return std::visit(
            [](const auto& value) -> ConstantValue {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return ConstantValue{std::monostate{}};
                } else if constexpr (std::is_same_v<T, bool>) {
                    return ConstantValue{value};
                } else if constexpr (std::is_same_v<T, Number>) {
                    return ConstantValue{value};
                } else if constexpr (std::is_same_v<T, Int>) {
                    return ConstantValue{value};
                } else if constexpr (std::is_same_v<T, String>) {
                    return ConstantValue{value};
                } else {
                    static_assert(sizeof(T) == 0, "Unsupported literal value type");
                }
            },
            literal_value);
    }

    String constant_value_to_string(const ConstantValue& value) {
        return std::visit(
            [](const auto& v) -> String {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return "nil";
                } else if constexpr (std::is_same_v<T, bool>) {
                    return v ? "true" : "false";
                } else if constexpr (std::is_same_v<T, Number>) {
                    return std::to_string(v);
                } else if constexpr (std::is_same_v<T, Int>) {
                    return std::to_string(v);
                } else if constexpr (std::is_same_v<T, String>) {
                    return "\"" + v + "\"";
                } else {
                    static_assert(sizeof(T) == 0, "Unsupported constant value type");
                }
            },
            value);
    }

    bool constant_values_equal(const ConstantValue& a, const ConstantValue& b) noexcept {
        return a == b;  // std::variant provides operator==
    }

    Size ConstantValueHash::operator()(const ConstantValue& value) const noexcept {
        return std::visit(
            [](const auto& v) -> Size {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return 0;
                } else if constexpr (std::is_same_v<T, bool>) {
                    return std::hash<bool>{}(v);
                } else if constexpr (std::is_same_v<T, Number>) {
                    return std::hash<Number>{}(v);
                } else if constexpr (std::is_same_v<T, Int>) {
                    return std::hash<Int>{}(v);
                } else if constexpr (std::is_same_v<T, String>) {
                    return std::hash<String>{}(v);
                } else {
                    return 0;
                }
            },
            value);
    }

    // InstructionEncoder methods are defined inline in the header

    // BytecodeEmitter implementation
    BytecodeEmitter::BytecodeEmitter(String function_name) {
        function_.name = std::move(function_name);
    }

    Size BytecodeEmitter::emit_abc(OpCode op, Register a, Register b, Register c) {
        return emit_instruction(InstructionEncoder::encode_abc(op, a, b, c));
    }

    Size BytecodeEmitter::emit_abx(OpCode op, Register a, std::uint32_t bx) {
        return emit_instruction(InstructionEncoder::encode_abx(op, a, bx));
    }

    Size BytecodeEmitter::emit_asbx(OpCode op, Register a, std::int32_t sbx) {
        return emit_instruction(InstructionEncoder::encode_asbx(op, a, sbx));
    }

    Size BytecodeEmitter::emit_ax(OpCode op, std::uint32_t ax) {
        return emit_instruction(InstructionEncoder::encode_ax(op, ax));
    }

    Size BytecodeEmitter::emit_instruction(Instruction instr) {
        Size index = function_.instructions.size();
        function_.instructions.push_back(instr);
        return index;
    }

    void BytecodeEmitter::patch_instruction(Size index, Instruction instr) {
        if (index < function_.instructions.size()) {
            function_.instructions[index] = instr;
        }
    }

    Size BytecodeEmitter::instruction_count() const noexcept {
        return function_.instructions.size();
    }

    Size BytecodeEmitter::add_constant(const String& value) {
        return add_constant(ConstantValue{value});
    }

    Size BytecodeEmitter::add_constant(const ConstantValue& value) {
        // For string constants, use the string map for deduplication
        if (is_string_constant(value)) {
            const String& str = std::get<String>(value);
            auto it = string_constant_map_.find(str);
            if (it != string_constant_map_.end()) {
                return it->second;
            }

            Size index = function_.constants.size();
            function_.constants.push_back(value);
            string_constant_map_[str] = index;
            return index;
        }

        // For other constant types, check if we already have this exact value
        for (Size i = 0; i < function_.constants.size(); ++i) {
            if (constant_values_equal(function_.constants[i], value)) {
                return i;
            }
        }

        // Add new constant
        Size index = function_.constants.size();
        function_.constants.push_back(value);
        return index;
    }

    Size BytecodeEmitter::add_local(const String& name) {
        auto it = local_map_.find(name);
        if (it != local_map_.end()) {
            return it->second;
        }

        Size index = function_.locals.size();
        function_.locals.push_back(name);
        local_map_[name] = index;
        return index;
    }

    Size BytecodeEmitter::add_upvalue(const String& name) {
        auto it = upvalue_map_.find(name);
        if (it != upvalue_map_.end()) {
            return it->second;
        }

        Size index = function_.upvalues.size();
        function_.upvalues.push_back(name);
        upvalue_map_[name] = index;
        return index;
    }

    Size
    BytecodeEmitter::add_upvalue_descriptor(const String& name, bool in_stack, std::uint8_t index) {
        UpvalueDescriptor descriptor(name, in_stack, index);
        return add_upvalue_descriptor(descriptor);
    }

    Size BytecodeEmitter::add_upvalue_descriptor(const UpvalueDescriptor& descriptor) {
        Size index = function_.upvalue_descriptors.size();
        function_.upvalue_descriptors.push_back(descriptor);
        return index;
    }

    Size BytecodeEmitter::add_prototype(const FunctionPrototype& prototype) {
        Size index = function_.prototypes.size();
        function_.prototypes.push_back(prototype);
        return index;
    }

    void BytecodeEmitter::set_parameter_count(Size count) {
        function_.parameter_count = count;
    }

    void BytecodeEmitter::set_stack_size(Size size) {
        function_.stack_size = size;
    }

    void BytecodeEmitter::set_vararg(bool is_vararg) {
        function_.is_vararg = is_vararg;
    }

    void BytecodeEmitter::add_line_info(Size line) {
        function_.line_info.push_back(line);
    }

    void BytecodeEmitter::set_source_name(String name) {
        function_.source_name = std::move(name);
    }

    BytecodeFunction BytecodeEmitter::get_function() const {
        return function_;
    }

    Span<const Instruction> BytecodeEmitter::instructions() const noexcept {
        return {function_.instructions};
    }

    void BytecodeEmitter::reset() {
        function_ = BytecodeFunction{};
        string_constant_map_.clear();
        local_map_.clear();
        upvalue_map_.clear();
    }

    // Disassembler implementation
    String Disassembler::disassemble_instruction(Instruction instr, Size index) {
        std::ostringstream oss;

        OpCode op = InstructionEncoder::decode_opcode(instr);
        Register a = InstructionEncoder::decode_a(instr);

        oss << std::setw(4) << index << ": " << opcode_name(op);

        switch (op) {
            // Simple load operations (no operands)
            case OpCode::OP_LOADFALSE:
            case OpCode::OP_LFALSESKIP:
            case OpCode::OP_LOADTRUE:
            case OpCode::OP_RETURN0:
            case OpCode::OP_CLOSE:
            case OpCode::OP_TBC:
                oss << " R" << static_cast<int>(a);
                break;

            // Load operations with B operand
            case OpCode::OP_MOVE:
            case OpCode::OP_GETUPVAL:
            case OpCode::OP_UNM:
            case OpCode::OP_NOT:
            case OpCode::OP_LEN:
            case OpCode::OP_BNOT: {
                Register b = InstructionEncoder::decode_b(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b);
                break;
            }

            // Load operations with B range (LOADNIL)
            case OpCode::OP_LOADNIL: {
                Register b = InstructionEncoder::decode_b(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b);
                break;
            }

            // Store operations (SETUPVAL)
            case OpCode::OP_SETUPVAL: {
                Register b = InstructionEncoder::decode_b(instr);
                oss << " R" << static_cast<int>(a) << " U" << static_cast<int>(b);
                break;
            }

            // Arithmetic operations with registers (ABC format)
            case OpCode::OP_ADD:
            case OpCode::OP_SUB:
            case OpCode::OP_MUL:
            case OpCode::OP_DIV:
            case OpCode::OP_IDIV:
            case OpCode::OP_MOD:
            case OpCode::OP_POW:
            case OpCode::OP_BAND:
            case OpCode::OP_BOR:
            case OpCode::OP_BXOR:
            case OpCode::OP_SHL:
            case OpCode::OP_SHR:
            case OpCode::OP_MMBIN: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " R"
                    << static_cast<int>(c);
                break;
            }

            // Arithmetic operations with constants (ABC format)
            case OpCode::OP_ADDK:
            case OpCode::OP_SUBK:
            case OpCode::OP_MULK:
            case OpCode::OP_MODK:
            case OpCode::OP_POWK:
            case OpCode::OP_DIVK:
            case OpCode::OP_IDIVK:
            case OpCode::OP_BANDK:
            case OpCode::OP_BORK:
            case OpCode::OP_BXORK:
            case OpCode::OP_MMBINK: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " K"
                    << static_cast<int>(c);
                break;
            }

            // Arithmetic operations with immediates (ABC format)
            case OpCode::OP_ADDI:
            case OpCode::OP_SHRI:
            case OpCode::OP_SHLI:
            case OpCode::OP_MMBINI: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            // Load operations with signed immediate (AsBx format)
            case OpCode::OP_LOADI:
            case OpCode::OP_LOADF: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                oss << " R" << static_cast<int>(a) << " " << sbx;
                break;
            }

            // Load operations with constant index (ABx format)
            case OpCode::OP_LOADK:
            case OpCode::OP_LOADKX: {
                std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                oss << " R" << static_cast<int>(a) << " K" << bx;
                break;
            }

            // Jump operations (AsBx format)
            case OpCode::OP_JMP: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                oss << " " << sbx << " (to " << (index + 1 + sbx) << ")";
                break;
            }

            // Loop operations (AsBx format)
            case OpCode::OP_FORPREP:
            case OpCode::OP_FORLOOP: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                oss << " R" << static_cast<int>(a) << " " << sbx;
                break;
            }

            // Table for loop operations (ABx format)
            case OpCode::OP_TFORPREP:
            case OpCode::OP_TFORLOOP: {
                std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                oss << " R" << static_cast<int>(a) << " " << bx;
                break;
            }

            // Function call operations (ABC format)
            case OpCode::OP_CALL:
            case OpCode::OP_TAILCALL:
            case OpCode::OP_TFORCALL: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            // Return operations
            case OpCode::OP_RETURN:
            case OpCode::OP_VARARG: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_RETURN1: {
                oss << " R" << static_cast<int>(a);
                break;
            }

            // Table operations
            case OpCode::OP_GETTABUP: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " U" << static_cast<int>(b) << " K"
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_GETTABLE:
            case OpCode::OP_SETTABLE: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " R"
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_GETI:
            case OpCode::OP_SETI: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_GETFIELD:
            case OpCode::OP_SETFIELD: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " K"
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_SETTABUP: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " U" << static_cast<int>(a) << " K" << static_cast<int>(b) << " R"
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_NEWTABLE: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_SELF: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " K"
                    << static_cast<int>(c);
                break;
            }

            // Comparison operations
            case OpCode::OP_EQ:
            case OpCode::OP_LT:
            case OpCode::OP_LE: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_EQK: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " K" << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_EQI:
            case OpCode::OP_LTI:
            case OpCode::OP_LEI:
            case OpCode::OP_GTI:
            case OpCode::OP_GEI: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_TEST: {
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(c);
                break;
            }

            case OpCode::OP_TESTSET: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            // String operations
            case OpCode::OP_CONCAT: {
                Register b = InstructionEncoder::decode_b(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b);
                break;
            }

            // List operations
            case OpCode::OP_SETLIST: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " " << static_cast<int>(b) << " "
                    << static_cast<int>(c);
                break;
            }

            // Closure operations
            case OpCode::OP_CLOSURE: {
                std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                oss << " R" << static_cast<int>(a) << " " << bx;
                break;
            }

            // Vararg operations
            case OpCode::OP_VARARGPREP: {
                oss << " R" << static_cast<int>(a);
                break;
            }

            // Extra argument
            case OpCode::OP_EXTRAARG: {
                std::uint32_t ax = InstructionEncoder::decode_ax(instr);
                oss << " " << ax;
                break;
            }

            default:
                oss << " R" << static_cast<int>(a);
                break;
        }

        return oss.str();
    }

    String Disassembler::disassemble_function(const BytecodeFunction& function) {
        std::ostringstream oss;

        oss << "Function: " << function.name << "\n";
        oss << "Parameters: " << function.parameter_count << "\n";
        oss << "Stack size: " << function.stack_size << "\n";
        oss << "Instructions:\n";

        for (Size i = 0; i < function.instructions.size(); ++i) {
            oss << disassemble_instruction(function.instructions[i], i) << "\n";
        }

        if (!function.constants.empty()) {
            oss << "Constants:\n";
            for (Size i = 0; i < function.constants.size(); ++i) {
                oss << "  K" << i << ": " << constant_value_to_string(function.constants[i])
                    << "\n";
            }
        }

        if (!function.prototypes.empty()) {
            oss << "Prototypes:\n";
            for (Size i = 0; i < function.prototypes.size(); ++i) {
                oss << "  P" << i << ": " << function.prototypes[i].name
                    << " (params: " << function.prototypes[i].parameter_count
                    << ", stack: " << function.prototypes[i].stack_size << ")\n";
                // Optionally show prototype instructions
                oss << "    Instructions:\n";
                for (Size j = 0; j < function.prototypes[i].instructions.size(); ++j) {
                    oss << "      "
                        << disassemble_instruction(function.prototypes[i].instructions[j], j)
                        << "\n";
                }
            }
        }

        return oss.str();
    }

    StringView Disassembler::opcode_name(OpCode op) noexcept {
        switch (op) {
            // Load operations
            case OpCode::OP_MOVE:
                return "MOVE";
            case OpCode::OP_LOADI:
                return "LOADI";
            case OpCode::OP_LOADF:
                return "LOADF";
            case OpCode::OP_LOADK:
                return "LOADK";
            case OpCode::OP_LOADKX:
                return "LOADKX";
            case OpCode::OP_LOADFALSE:
                return "LOADFALSE";
            case OpCode::OP_LFALSESKIP:
                return "LFALSESKIP";
            case OpCode::OP_LOADTRUE:
                return "LOADTRUE";
            case OpCode::OP_LOADNIL:
                return "LOADNIL";

            // Upvalue operations
            case OpCode::OP_GETUPVAL:
                return "GETUPVAL";
            case OpCode::OP_SETUPVAL:
                return "SETUPVAL";

            // Table operations
            case OpCode::OP_GETTABUP:
                return "GETTABUP";
            case OpCode::OP_GETTABLE:
                return "GETTABLE";
            case OpCode::OP_GETI:
                return "GETI";
            case OpCode::OP_GETFIELD:
                return "GETFIELD";
            case OpCode::OP_SETTABUP:
                return "SETTABUP";
            case OpCode::OP_SETTABLE:
                return "SETTABLE";
            case OpCode::OP_SETI:
                return "SETI";
            case OpCode::OP_SETFIELD:
                return "SETFIELD";
            case OpCode::OP_NEWTABLE:
                return "NEWTABLE";
            case OpCode::OP_SELF:
                return "SELF";

            // Arithmetic operations with constants/immediates
            case OpCode::OP_ADDI:
                return "ADDI";
            case OpCode::OP_ADDK:
                return "ADDK";
            case OpCode::OP_SUBK:
                return "SUBK";
            case OpCode::OP_MULK:
                return "MULK";
            case OpCode::OP_MODK:
                return "MODK";
            case OpCode::OP_POWK:
                return "POWK";
            case OpCode::OP_DIVK:
                return "DIVK";
            case OpCode::OP_IDIVK:
                return "IDIVK";

            // Bitwise operations with constants/immediates
            case OpCode::OP_BANDK:
                return "BANDK";
            case OpCode::OP_BORK:
                return "BORK";
            case OpCode::OP_BXORK:
                return "BXORK";
            case OpCode::OP_SHRI:
                return "SHRI";
            case OpCode::OP_SHLI:
                return "SHLI";

            // Arithmetic operations with registers
            case OpCode::OP_ADD:
                return "ADD";
            case OpCode::OP_SUB:
                return "SUB";
            case OpCode::OP_MUL:
                return "MUL";
            case OpCode::OP_MOD:
                return "MOD";
            case OpCode::OP_POW:
                return "POW";
            case OpCode::OP_DIV:
                return "DIV";
            case OpCode::OP_IDIV:
                return "IDIV";

            // Bitwise operations with registers
            case OpCode::OP_BAND:
                return "BAND";
            case OpCode::OP_BOR:
                return "BOR";
            case OpCode::OP_BXOR:
                return "BXOR";
            case OpCode::OP_SHL:
                return "SHL";
            case OpCode::OP_SHR:
                return "SHR";

            // Metamethod operations
            case OpCode::OP_MMBIN:
                return "MMBIN";
            case OpCode::OP_MMBINI:
                return "MMBINI";
            case OpCode::OP_MMBINK:
                return "MMBINK";

            // Unary operations
            case OpCode::OP_UNM:
                return "UNM";
            case OpCode::OP_BNOT:
                return "BNOT";
            case OpCode::OP_NOT:
                return "NOT";
            case OpCode::OP_LEN:
                return "LEN";

            case OpCode::OP_CONCAT:
                return "CONCAT";

            // Control flow
            case OpCode::OP_CLOSE:
                return "CLOSE";
            case OpCode::OP_TBC:
                return "TBC";
            case OpCode::OP_JMP:
                return "JMP";

            // Comparison operations
            case OpCode::OP_EQ:
                return "EQ";
            case OpCode::OP_LT:
                return "LT";
            case OpCode::OP_LE:
                return "LE";
            case OpCode::OP_EQK:
                return "EQK";
            case OpCode::OP_EQI:
                return "EQI";
            case OpCode::OP_LTI:
                return "LTI";
            case OpCode::OP_LEI:
                return "LEI";
            case OpCode::OP_GTI:
                return "GTI";
            case OpCode::OP_GEI:
                return "GEI";

            case OpCode::OP_TEST:
                return "TEST";
            case OpCode::OP_TESTSET:
                return "TESTSET";

            // Function operations
            case OpCode::OP_CALL:
                return "CALL";
            case OpCode::OP_TAILCALL:
                return "TAILCALL";
            case OpCode::OP_RETURN:
                return "RETURN";
            case OpCode::OP_RETURN0:
                return "RETURN0";
            case OpCode::OP_RETURN1:
                return "RETURN1";

            // Loop operations
            case OpCode::OP_FORLOOP:
                return "FORLOOP";
            case OpCode::OP_FORPREP:
                return "FORPREP";
            case OpCode::OP_TFORPREP:
                return "TFORPREP";
            case OpCode::OP_TFORCALL:
                return "TFORCALL";
            case OpCode::OP_TFORLOOP:
                return "TFORLOOP";

            case OpCode::OP_SETLIST:
                return "SETLIST";
            case OpCode::OP_CLOSURE:
                return "CLOSURE";
            case OpCode::OP_VARARG:
                return "VARARG";
            case OpCode::OP_VARARGPREP:
                return "VARARGPREP";
            case OpCode::OP_EXTRAARG:
                return "EXTRAARG";

            default:
                return "UNKNOWN";
        }
    }

    StringView Disassembler::instruction_format(OpCode op) noexcept {
        switch (op) {
            // ABx format instructions
            case OpCode::OP_LOADK:
            case OpCode::OP_LOADKX:
            case OpCode::OP_FORLOOP:
            case OpCode::OP_FORPREP:
            case OpCode::OP_TFORPREP:
            case OpCode::OP_TFORLOOP:
            case OpCode::OP_CLOSURE:
                return "ABx";

            // AsBx format instructions
            case OpCode::OP_LOADI:
            case OpCode::OP_LOADF:
            case OpCode::OP_JMP:
                return "AsBx";

            // Ax format instructions
            case OpCode::OP_EXTRAARG:
                return "Ax";

            // ABC format instructions (default)
            default:
                return "ABC";
        }
    }

    // BytecodeValidator implementation
    BytecodeValidator::ValidationResult
    BytecodeValidator::validate(const BytecodeFunction& function) noexcept {
        std::vector<ValidationError> errors;

        try {
            // Validate basic function properties
            if (function.instructions.empty()) {
                errors.emplace_back(0, "Function has no instructions", ErrorCode::RUNTIME_ERROR);
                return errors;
            }

            // Validate each instruction
            for (Size i = 0; i < function.instructions.size(); ++i) {
                auto instr_result = validate_instruction(function.instructions[i], function, i);
                if (is_error(instr_result)) {
                    errors.emplace_back(i, "Invalid instruction", get_error(instr_result));
                }
            }

            // Validate stack size requirements
            if (function.stack_size == 0) {
                errors.emplace_back(0, "Function has zero stack size", ErrorCode::RUNTIME_ERROR);
            }

            // Validate parameter count
            if (function.parameter_count > function.stack_size) {
                errors.emplace_back(
                    0, "Parameter count exceeds stack size", ErrorCode::RUNTIME_ERROR);
            }

            return errors;

        } catch (...) {
            // Ensure no exceptions escape from noexcept function
            errors.emplace_back(0, "Internal validation error", ErrorCode::UNKNOWN_ERROR);
            return errors;
        }
    }

    Result<std::monostate> BytecodeValidator::validate_instruction(Instruction instr,
                                                                   const BytecodeFunction& function,
                                                                   Size index) noexcept {
        try {
            // Validate instruction format
            auto format_result = validate_instruction_format(instr, index);
            if (is_error(format_result)) {
                return get_error(format_result);
            }

            // Validate register usage
            auto register_result = validate_register_usage(instr, function, index);
            if (is_error(register_result)) {
                return get_error(register_result);
            }

            // Validate constant usage
            auto constant_result = validate_constant_usage(instr, function, index);
            if (is_error(constant_result)) {
                return get_error(constant_result);
            }

            // Validate jump targets
            auto jump_result = validate_jump_targets(instr, function, index);
            if (is_error(jump_result)) {
                return get_error(jump_result);
            }

            return std::monostate{};

        } catch (...) {
            // Ensure no exceptions escape from noexcept function
            return ErrorCode::UNKNOWN_ERROR;
        }
    }

    // Private helper method implementations
    Result<std::monostate>
    BytecodeValidator::validate_instruction_format(Instruction instr, Size /* index */) noexcept {
        try {
            OpCode opcode = InstructionEncoder::decode_opcode(instr);

            // Validate opcode is within valid range
            if (static_cast<std::uint8_t>(opcode) >=
                static_cast<std::uint8_t>(OpCode::OP_EXTRAARG)) {
                return ErrorCode::RUNTIME_ERROR;
            }

            // Validate instruction fields based on opcode format
            Register a = InstructionEncoder::decode_a(instr);
            if (a > InstructionEncoder::MAX_A) {
                return ErrorCode::RUNTIME_ERROR;
            }

            // Additional format validation can be added here based on instruction type
            return std::monostate{};

        } catch (...) {
            return ErrorCode::UNKNOWN_ERROR;
        }
    }

    Result<std::monostate> BytecodeValidator::validate_register_usage(
        Instruction instr, const BytecodeFunction& function, Size /* index */) noexcept {
        try {
            OpCode opcode = InstructionEncoder::decode_opcode(instr);
            Register a = InstructionEncoder::decode_a(instr);

            // Validate register A is within stack bounds
            if (a >= function.stack_size) {
                return ErrorCode::RUNTIME_ERROR;
            }

            // For instructions that use B and C registers, validate them too
            switch (opcode) {
                // ABC format instructions with register operands
                case OpCode::OP_MOVE:
                case OpCode::OP_ADD:
                case OpCode::OP_SUB:
                case OpCode::OP_MUL:
                case OpCode::OP_DIV:
                case OpCode::OP_IDIV:
                case OpCode::OP_MOD:
                case OpCode::OP_POW:
                case OpCode::OP_BAND:
                case OpCode::OP_BOR:
                case OpCode::OP_BXOR:
                case OpCode::OP_SHL:
                case OpCode::OP_SHR:
                case OpCode::OP_MMBIN:
                case OpCode::OP_GETTABLE:
                case OpCode::OP_SETTABLE:
                case OpCode::OP_GETI:
                case OpCode::OP_SETI:
                case OpCode::OP_EQ:
                case OpCode::OP_LT:
                case OpCode::OP_LE:
                case OpCode::OP_TESTSET: {
                    Register b = InstructionEncoder::decode_b(instr);
                    Register c = InstructionEncoder::decode_c(instr);

                    if (b >= function.stack_size || c >= function.stack_size) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // ABC format instructions with constant operands (don't validate C as register)
                case OpCode::OP_ADDK:
                case OpCode::OP_SUBK:
                case OpCode::OP_MULK:
                case OpCode::OP_MODK:
                case OpCode::OP_POWK:
                case OpCode::OP_DIVK:
                case OpCode::OP_IDIVK:
                case OpCode::OP_BANDK:
                case OpCode::OP_BORK:
                case OpCode::OP_BXORK:
                case OpCode::OP_MMBINK:
                case OpCode::OP_GETFIELD:
                case OpCode::OP_SETFIELD:
                case OpCode::OP_GETTABUP:
                case OpCode::OP_SETTABUP:
                case OpCode::OP_SELF:
                case OpCode::OP_EQK: {
                    Register b = InstructionEncoder::decode_b(instr);
                    if (b >= function.stack_size) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // ABC format instructions with immediate operands
                case OpCode::OP_ADDI:
                case OpCode::OP_SHRI:
                case OpCode::OP_SHLI:
                case OpCode::OP_MMBINI:
                case OpCode::OP_EQI:
                case OpCode::OP_LTI:
                case OpCode::OP_LEI:
                case OpCode::OP_GTI:
                case OpCode::OP_GEI: {
                    Register b = InstructionEncoder::decode_b(instr);
                    if (b >= function.stack_size) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // Instructions with only B register
                case OpCode::OP_GETUPVAL:
                case OpCode::OP_SETUPVAL:
                case OpCode::OP_UNM:
                case OpCode::OP_NOT:
                case OpCode::OP_LEN:
                case OpCode::OP_BNOT:
                case OpCode::OP_LOADNIL:
                case OpCode::OP_CONCAT: {
                    Register b = InstructionEncoder::decode_b(instr);
                    if (b >= function.stack_size) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // Function call instructions
                case OpCode::OP_CALL:
                case OpCode::OP_TAILCALL:
                case OpCode::OP_TFORCALL:
                case OpCode::OP_RETURN:
                case OpCode::OP_VARARG:
                case OpCode::OP_NEWTABLE:
                case OpCode::OP_SETLIST: {
                    [[maybe_unused]] Register b = InstructionEncoder::decode_b(instr);
                    [[maybe_unused]] Register c = InstructionEncoder::decode_c(instr);
                    // For these instructions, B and C may be special values, not just register indices
                    // Additional validation logic could be added here
                    break;
                }

                // Instructions with only C register (TEST)
                case OpCode::OP_TEST: {
                    [[maybe_unused]] Register c = InstructionEncoder::decode_c(instr);
                    // C is a boolean flag for TEST, not a register
                    break;
                }

                default:
                    // Other instructions may have different register usage patterns
                    break;
            }

            return std::monostate{};

        } catch (...) {
            return ErrorCode::UNKNOWN_ERROR;
        }
    }

    Result<std::monostate> BytecodeValidator::validate_constant_usage(
        Instruction instr, const BytecodeFunction& function, Size /* index */) noexcept {
        try {
            OpCode opcode = InstructionEncoder::decode_opcode(instr);

            // For instructions that reference constants, validate the constant index
            switch (opcode) {
                // ABx format instructions that use constants
                case OpCode::OP_LOADK:
                case OpCode::OP_LOADKX: {
                    std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                    if (bx >= function.constants.size()) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // ABC format instructions that use constants in C field
                case OpCode::OP_ADDK:
                case OpCode::OP_SUBK:
                case OpCode::OP_MULK:
                case OpCode::OP_MODK:
                case OpCode::OP_POWK:
                case OpCode::OP_DIVK:
                case OpCode::OP_IDIVK:
                case OpCode::OP_BANDK:
                case OpCode::OP_BORK:
                case OpCode::OP_BXORK:
                case OpCode::OP_MMBINK:
                case OpCode::OP_GETFIELD:
                case OpCode::OP_SETFIELD:
                case OpCode::OP_GETTABUP:
                case OpCode::OP_SETTABUP:
                case OpCode::OP_SELF:
                case OpCode::OP_EQK: {
                    Register c = InstructionEncoder::decode_c(instr);
                    if (c >= function.constants.size()) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // ABx format instructions that reference function prototypes
                case OpCode::OP_CLOSURE: {
                    std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                    // TODO: Validate function prototype index when prototypes are implemented
                    (void)bx; // Suppress unused variable warning
                    break;
                }

                default:
                    // Other instructions may not use constants
                    break;
            }

            return std::monostate{};

        } catch (...) {
            return ErrorCode::UNKNOWN_ERROR;
        }
    }

    Result<std::monostate> BytecodeValidator::validate_jump_targets(
        Instruction instr, const BytecodeFunction& function, Size index) noexcept {
        try {
            OpCode opcode = InstructionEncoder::decode_opcode(instr);

            // For jump instructions, validate the target is within bounds
            switch (opcode) {
                // Unconditional jump
                case OpCode::OP_JMP: {
                    std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                    std::int64_t target = static_cast<std::int64_t>(index) + 1 + sbx;

                    if (target < 0 ||
                        target >= static_cast<std::int64_t>(function.instructions.size())) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // Loop instructions with backward jumps
                case OpCode::OP_FORLOOP: {
                    std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                    std::int64_t target = static_cast<std::int64_t>(index) + 1 - sbx;

                    if (target < 0 ||
                        target >= static_cast<std::int64_t>(function.instructions.size())) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // Loop preparation with forward jumps
                case OpCode::OP_FORPREP: {
                    std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                    std::int64_t target = static_cast<std::int64_t>(index) + 1 + sbx;

                    if (target < 0 ||
                        target >= static_cast<std::int64_t>(function.instructions.size())) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // Table for loop instructions
                case OpCode::OP_TFORPREP: {
                    std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                    std::int64_t target = static_cast<std::int64_t>(index) + 1 + bx;

                    if (target >= static_cast<std::int64_t>(function.instructions.size())) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                case OpCode::OP_TFORLOOP: {
                    std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                    std::int64_t target = static_cast<std::int64_t>(index) + 1 - bx;

                    if (target < 0) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                // Conditional jumps (these skip the next instruction if condition is false)
                case OpCode::OP_EQ:
                case OpCode::OP_LT:
                case OpCode::OP_LE:
                case OpCode::OP_EQK:
                case OpCode::OP_EQI:
                case OpCode::OP_LTI:
                case OpCode::OP_LEI:
                case OpCode::OP_GTI:
                case OpCode::OP_GEI:
                case OpCode::OP_TEST:
                case OpCode::OP_TESTSET: {
                    // These instructions conditionally skip the next instruction
                    std::int64_t target = static_cast<std::int64_t>(index) + 2;

                    if (target >= static_cast<std::int64_t>(function.instructions.size())) {
                        return ErrorCode::RUNTIME_ERROR;
                    }
                    break;
                }

                default:
                    // Other instructions may not be jumps
                    break;
            }

            return std::monostate{};

        } catch (...) {
            return ErrorCode::UNKNOWN_ERROR;
        }
    }

}  // namespace rangelua::backend
