/**
 * @file bytecode.cpp
 * @brief Bytecode implementation
 * @version 0.1.0
 */

#include <rangelua/backend/bytecode.hpp>

#include <iomanip>
#include <sstream>

namespace rangelua::backend {

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
        auto it = constant_map_.find(value);
        if (it != constant_map_.end()) {
            return it->second;
        }

        Size index = function_.constants.size();
        function_.constants.push_back(value);
        constant_map_[value] = index;
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
        constant_map_.clear();
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
            case OpCode::OP_MOVE:
            case OpCode::OP_UNM:
            case OpCode::OP_NOT:
            case OpCode::OP_LEN:
            case OpCode::OP_BNOT: {
                Register b = InstructionEncoder::decode_b(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b);
                break;
            }

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
            case OpCode::OP_SHR: {
                Register b = InstructionEncoder::decode_b(instr);
                Register c = InstructionEncoder::decode_c(instr);
                oss << " R" << static_cast<int>(a) << " R" << static_cast<int>(b) << " R"
                    << static_cast<int>(c);
                break;
            }

            case OpCode::OP_LOADI:
            case OpCode::OP_LOADF: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                oss << " R" << static_cast<int>(a) << " " << sbx;
                break;
            }

            case OpCode::OP_LOADK: {
                std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                oss << " R" << static_cast<int>(a) << " K" << bx;
                break;
            }

            case OpCode::OP_JMP: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                oss << " " << sbx << " (to " << (index + 1 + sbx) << ")";
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
                oss << "  K" << i << ": \"" << function.constants[i] << "\"\n";
            }
        }

        return oss.str();
    }

    StringView Disassembler::opcode_name(OpCode op) noexcept {
        switch (op) {
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
            case OpCode::OP_LOADTRUE:
                return "LOADTRUE";
            case OpCode::OP_LOADNIL:
                return "LOADNIL";
            case OpCode::OP_ADD:
                return "ADD";
            case OpCode::OP_SUB:
                return "SUB";
            case OpCode::OP_MUL:
                return "MUL";
            case OpCode::OP_DIV:
                return "DIV";
            case OpCode::OP_IDIV:
                return "IDIV";
            case OpCode::OP_MOD:
                return "MOD";
            case OpCode::OP_POW:
                return "POW";
            case OpCode::OP_UNM:
                return "UNM";
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
            case OpCode::OP_BNOT:
                return "BNOT";
            case OpCode::OP_EQ:
                return "EQ";
            case OpCode::OP_LT:
                return "LT";
            case OpCode::OP_LE:
                return "LE";
            case OpCode::OP_NOT:
                return "NOT";
            case OpCode::OP_LEN:
                return "LEN";
            case OpCode::OP_GETTABLE:
                return "GETTABLE";
            case OpCode::OP_SETTABLE:
                return "SETTABLE";
            case OpCode::OP_NEWTABLE:
                return "NEWTABLE";
            case OpCode::OP_SELF:
                return "SELF";
            case OpCode::OP_CALL:
                return "CALL";
            case OpCode::OP_TAILCALL:
                return "TAILCALL";
            case OpCode::OP_RETURN:
                return "RETURN";
            case OpCode::OP_JMP:
                return "JMP";
            case OpCode::OP_TEST:
                return "TEST";
            case OpCode::OP_TESTSET:
                return "TESTSET";
            default:
                return "UNKNOWN";
        }
    }

    StringView Disassembler::instruction_format(OpCode /* op */) noexcept {
        // TODO: Implement instruction format descriptions
        return "ABC";
    }

    // BytecodeValidator implementation
    std::vector<BytecodeValidator::ValidationError> BytecodeValidator::validation_errors_;

    Result<std::monostate> BytecodeValidator::validate(const BytecodeFunction& /* function */) {
        validation_errors_.clear();

        // TODO: Implement bytecode validation

        if (validation_errors_.empty()) {
            return std::monostate{};
        }
        return ErrorCode::RUNTIME_ERROR;
    }

    Result<std::monostate> BytecodeValidator::validate_instruction(
        Instruction /* instr */, const BytecodeFunction& /* function */, Size /* index */) {
        // TODO: Implement instruction validation
        return std::monostate{};
    }

    const std::vector<BytecodeValidator::ValidationError>& BytecodeValidator::errors() noexcept {
        return validation_errors_;
    }

}  // namespace rangelua::backend
