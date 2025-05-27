/**
 * @file test_codegen_basic.cpp
 * @brief Basic tests for the code generation system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/backend/codegen.hpp>
#include <rangelua/frontend/ast.hpp>

using namespace rangelua;
using namespace rangelua::backend;
using namespace rangelua::frontend;

TEST_CASE("BytecodeEmitter basic functionality", "[codegen][bytecode]") {
    SECTION("Create and emit basic instructions") {
        backend::BytecodeEmitter emitter("test_function");

        // Test basic instruction emission
        Size instr1 = emitter.emit_abc(OpCode::OP_LOADNIL, 0, 0, 0);
        Size instr2 = emitter.emit_abc(OpCode::OP_LOADTRUE, 1, 0, 0);
        Size instr3 = emitter.emit_abc(OpCode::OP_LOADFALSE, 2, 0, 0);

        REQUIRE(instr1 == 0);
        REQUIRE(instr2 == 1);
        REQUIRE(instr3 == 2);
        REQUIRE(emitter.instruction_count() == 3);

        // Test constant addition
        Size const1 = emitter.add_constant(ConstantValue{Int{42}});
        Size const2 = emitter.add_constant(ConstantValue{String{"hello"}});
        Size const3 = emitter.add_constant(ConstantValue{Number{3.14}});

        REQUIRE(const1 == 0);
        REQUIRE(const2 == 1);
        REQUIRE(const3 == 2);

        // Test function generation
        auto function = emitter.get_function();
        REQUIRE(function.name == "test_function");
        REQUIRE(function.instructions.size() == 3);
        REQUIRE(function.constants.size() == 3);
    }

    SECTION("Constant deduplication") {
        backend::BytecodeEmitter emitter("test_dedup");

        // Add same string constant multiple times
        Size const1 = emitter.add_constant(ConstantValue{String{"test"}});
        Size const2 = emitter.add_constant(ConstantValue{String{"test"}});
        Size const3 = emitter.add_constant(ConstantValue{String{"different"}});

        REQUIRE(const1 == const2);  // Should be deduplicated
        REQUIRE(const1 != const3);  // Different string should get new index

        auto function = emitter.get_function();
        REQUIRE(function.constants.size() == 2);  // Only 2 unique constants
    }
}

TEST_CASE("RegisterAllocator functionality", "[codegen][register]") {
    SECTION("Basic allocation and deallocation") {
        RegisterAllocator allocator(10);

        // Test basic allocation
        auto reg1_result = allocator.allocate();
        REQUIRE(is_success(reg1_result));
        Register reg1 = get_value(reg1_result);

        auto reg2_result = allocator.allocate();
        REQUIRE(is_success(reg2_result));
        Register reg2 = get_value(reg2_result);

        REQUIRE(reg1 != reg2);
        REQUIRE(allocator.allocated_count() == 2);
        REQUIRE(allocator.high_water_mark() >= std::max(reg1, reg2));

        // Test deallocation and reuse
        allocator.free(reg1);
        REQUIRE(allocator.allocated_count() == 1);

        auto reg3_result = allocator.allocate();
        REQUIRE(is_success(reg3_result));
        Register reg3 = get_value(reg3_result);

        // Should reuse the freed register
        REQUIRE(reg3 == reg1);
        REQUIRE(allocator.allocated_count() == 2);
    }

    SECTION("Specific allocation") {
        RegisterAllocator allocator(10);

        // Test specific allocation
        auto status = allocator.allocate_specific(5);
        REQUIRE(is_success(status));
        REQUIRE(allocator.is_allocated(5));

        // Try to allocate same register again - should fail
        auto status2 = allocator.allocate_specific(5);
        REQUIRE(is_error(status2));
    }

    SECTION("Register reservation") {
        RegisterAllocator allocator(10);

        // Reserve a register
        allocator.reserve(3);

        // Try to allocate the reserved register - should skip it
        auto reg_result = allocator.allocate();
        REQUIRE(is_success(reg_result));
        Register reg = get_value(reg_result);
        REQUIRE(reg != 3);  // Should not allocate reserved register

        // Try to specifically allocate reserved register - should fail
        auto status = allocator.allocate_specific(3);
        REQUIRE(is_error(status));
    }
}

TEST_CASE("ScopeManager functionality", "[codegen][scope]") {
    SECTION("Basic scope management") {
        ScopeManager scope_manager;

        REQUIRE(scope_manager.scope_depth() == 0);

        // Enter scope and declare variables
        scope_manager.enter_scope();
        REQUIRE(scope_manager.scope_depth() == 1);

        Size local1 = scope_manager.declare_local("x", 0);
        Size local2 = scope_manager.declare_local("y", 1);

        REQUIRE(local1 == 0);
        REQUIRE(local2 == 1);
        REQUIRE(scope_manager.current_locals().size() == 2);

        // Test variable resolution
        auto resolution = scope_manager.resolve_variable("x");
        REQUIRE(resolution.type == ScopeManager::VariableResolution::Type::Local);
        REQUIRE(resolution.index == 0);

        // Exit scope
        scope_manager.exit_scope();
        REQUIRE(scope_manager.scope_depth() == 0);
        REQUIRE(scope_manager.current_locals().size() == 0);

        // Variable should no longer be found
        auto resolution2 = scope_manager.resolve_variable("x");
        REQUIRE(resolution2.type == ScopeManager::VariableResolution::Type::Global);
    }
}

TEST_CASE("CodeGenerator basic expression generation", "[codegen][expression]") {
    SECTION("Literal expression generation") {
        backend::BytecodeEmitter emitter("test_literals");
        backend::CodeGenerator generator(emitter);

        // Test nil literal
        auto nil_literal = std::make_unique<LiteralExpression>(std::monostate{});
        generator.visit(*nil_literal);

        // Test boolean literals
        auto true_literal = std::make_unique<LiteralExpression>(true);
        generator.visit(*true_literal);

        auto false_literal = std::make_unique<LiteralExpression>(false);
        generator.visit(*false_literal);

        // Test integer literal (use large value to force constant pool usage)
        auto int_literal = std::make_unique<LiteralExpression>(Int{100000});
        generator.visit(*int_literal);

        // Test string literal
        auto string_literal = std::make_unique<LiteralExpression>(String{"hello"});
        generator.visit(*string_literal);

        // Verify instructions were generated
        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 5);

        // Verify some constants were added
        REQUIRE(function.constants.size() >= 2);  // At least int and string constants
    }
}

TEST_CASE("Bytecode validation", "[codegen][validation]") {
    SECTION("Valid bytecode passes validation") {
        backend::BytecodeEmitter emitter("test_validation");

        // Generate some valid bytecode
        emitter.emit_abc(OpCode::OP_LOADNIL, 0, 0, 0);
        emitter.emit_abc(OpCode::OP_LOADTRUE, 1, 0, 0);
        emitter.emit_abc(OpCode::OP_ADD, 2, 0, 1);
        emitter.emit_abc(OpCode::OP_RETURN, 2, 1, 0);

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto validation_result = BytecodeValidator::validate(function);

        REQUIRE(is_success(validation_result));
        REQUIRE(BytecodeValidator::is_valid(validation_result));
    }
}

TEST_CASE("Disassembler functionality", "[codegen][disassembly]") {
    SECTION("Instruction disassembly") {
        // Test individual instruction disassembly
        Instruction move_instr = InstructionEncoder::encode_abc(OpCode::OP_MOVE, 1, 2, 0);
        String disasm = Disassembler::disassemble_instruction(move_instr, 0);

        REQUIRE(!disasm.empty());
        REQUIRE(disasm.find("MOVE") != String::npos);
    }

    SECTION("Function disassembly") {
        backend::BytecodeEmitter emitter("test_disasm");

        emitter.emit_abc(OpCode::OP_LOADNIL, 0, 0, 0);
        emitter.emit_abc(OpCode::OP_RETURN, 0, 1, 0);
        emitter.add_constant(ConstantValue{String{"test"}});

        auto function = emitter.get_function();
        String disasm = Disassembler::disassemble_function(function);

        REQUIRE(!disasm.empty());
        REQUIRE(disasm.find("test_disasm") != String::npos);
        REQUIRE(disasm.find("LOADNIL") != String::npos);
        REQUIRE(disasm.find("RETURN") != String::npos);
        REQUIRE(disasm.find("Constants:") != String::npos);
    }
}
