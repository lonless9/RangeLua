/**
 * @file test_codegen_comprehensive.cpp
 * @brief Comprehensive tests for the code generation system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/backend/codegen.hpp>
#include <rangelua/frontend/ast.hpp>

using namespace rangelua;
using namespace rangelua::backend;
using namespace rangelua::frontend;

TEST_CASE("CodeGenerator comprehensive statement generation", "[codegen][statements]") {
    SECTION("Local declaration with values") {
        backend::BytecodeEmitter emitter("test_local_decl");
        backend::CodeGenerator generator(emitter);

        // Create local declaration: local x, y = 42, "hello"
        std::vector<String> names = {"x", "y"};
        ExpressionList values;
        values.push_back(std::make_unique<LiteralExpression>(Int{42}));
        values.push_back(std::make_unique<LiteralExpression>(String{"hello"}));

        auto local_decl = std::make_unique<LocalDeclarationStatement>(
            std::move(names), std::move(values));

        generator.visit(*local_decl);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 4);  // At least 4 instructions
        REQUIRE(function.constants.size() >= 1);     // At least 1 constant (strings are deduplicated)
    }

    SECTION("Assignment statement") {
        backend::BytecodeEmitter emitter("test_assignment");
        backend::CodeGenerator generator(emitter);

        // Create assignment: x = 100
        ExpressionList targets;
        targets.push_back(std::make_unique<IdentifierExpression>("x"));

        ExpressionList values;
        values.push_back(std::make_unique<LiteralExpression>(Int{100}));

        auto assignment = std::make_unique<AssignmentStatement>(
            std::move(targets), std::move(values));

        generator.visit(*assignment);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 2);  // At least 2 instructions
        REQUIRE(function.constants.size() >= 1);     // At least 1 constant
    }

    SECTION("If statement") {
        backend::BytecodeEmitter emitter("test_if");
        backend::CodeGenerator generator(emitter);

        // Create if statement: if true then x = 1 end
        auto condition = std::make_unique<LiteralExpression>(true);

        StatementList then_stmts;
        ExpressionList targets;
        targets.push_back(std::make_unique<IdentifierExpression>("x"));
        ExpressionList values;
        values.push_back(std::make_unique<LiteralExpression>(Int{1}));
        then_stmts.push_back(std::make_unique<AssignmentStatement>(
            std::move(targets), std::move(values)));

        auto then_body = std::make_unique<BlockStatement>(std::move(then_stmts));

        auto if_stmt = std::make_unique<IfStatement>(
            std::move(condition), std::move(then_body));

        generator.visit(*if_stmt);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 4);  // At least 4 instructions for if
    }

    SECTION("While loop") {
        backend::BytecodeEmitter emitter("test_while");
        backend::CodeGenerator generator(emitter);

        // Create while loop: while true do x = x + 1 end
        auto condition = std::make_unique<LiteralExpression>(true);

        StatementList body_stmts;
        ExpressionList targets;
        targets.push_back(std::make_unique<IdentifierExpression>("x"));
        ExpressionList values;
        auto add_expr = std::make_unique<BinaryOpExpression>(
            BinaryOpExpression::Operator::Add,
            std::make_unique<IdentifierExpression>("x"),
            std::make_unique<LiteralExpression>(Int{1}));
        values.push_back(std::move(add_expr));
        body_stmts.push_back(std::make_unique<AssignmentStatement>(
            std::move(targets), std::move(values)));

        auto body = std::make_unique<BlockStatement>(std::move(body_stmts));

        auto while_stmt = std::make_unique<WhileStatement>(
            std::move(condition), std::move(body));

        generator.visit(*while_stmt);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 6);  // At least 6 instructions for while loop
    }

    SECTION("Return statement") {
        backend::BytecodeEmitter emitter("test_return");
        backend::CodeGenerator generator(emitter);

        // Create return statement: return 42, "hello"
        ExpressionList values;
        values.push_back(std::make_unique<LiteralExpression>(Int{42}));
        values.push_back(std::make_unique<LiteralExpression>(String{"hello"}));

        auto return_stmt = std::make_unique<ReturnStatement>(std::move(values));

        generator.visit(*return_stmt);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 3);  // At least 3 instructions
        REQUIRE(function.constants.size() >= 1);     // At least 1 constant (strings are deduplicated)
    }
}

TEST_CASE("CodeGenerator expression generation", "[codegen][expressions]") {
    SECTION("Function call expression") {
        backend::BytecodeEmitter emitter("test_func_call");
        backend::CodeGenerator generator(emitter);

        // Create function call: print("hello")
        auto func = std::make_unique<IdentifierExpression>("print");
        ExpressionList args;
        args.push_back(std::make_unique<LiteralExpression>(String{"hello"}));

        auto call_expr = std::make_unique<FunctionCallExpression>(
            std::move(func), std::move(args));

        generator.visit(*call_expr);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 3);  // At least 3 instructions
        REQUIRE(function.constants.size() >= 1);     // At least 1 constant
    }

    SECTION("Table access expression") {
        backend::BytecodeEmitter emitter("test_table_access");
        backend::CodeGenerator generator(emitter);

        // Create table access: t["key"]
        auto table = std::make_unique<IdentifierExpression>("t");
        auto key = std::make_unique<LiteralExpression>(String{"key"});

        auto access_expr = std::make_unique<TableAccessExpression>(
            std::move(table), std::move(key));

        generator.visit(*access_expr);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 3);  // At least 3 instructions
        REQUIRE(function.constants.size() >= 1);     // At least 1 constant
    }

    SECTION("Table constructor expression") {
        backend::BytecodeEmitter emitter("test_table_constructor");
        backend::CodeGenerator generator(emitter);

        // Create table constructor: {}
        TableConstructorExpression::FieldList fields;

        auto table_expr = std::make_unique<TableConstructorExpression>(std::move(fields));

        generator.visit(*table_expr);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() >= 1);  // At least 1 instruction (NEWTABLE)
    }
}

TEST_CASE("CodeGenerator bytecode validation", "[codegen][validation]") {
    SECTION("Generated bytecode is valid") {
        backend::BytecodeEmitter emitter("test_validation");
        backend::CodeGenerator generator(emitter);

        // Generate a simple program
        auto literal = std::make_unique<LiteralExpression>(Int{42});
        generator.visit(*literal);

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto validation_result = BytecodeValidator::validate(function);

        REQUIRE(is_success(validation_result));
        REQUIRE(BytecodeValidator::is_valid(validation_result));
    }
}

TEST_CASE("CodeGenerator constant deduplication", "[codegen][constants]") {
    SECTION("Test constant deduplication with same values") {
        backend::BytecodeEmitter emitter("test_dedup");
        backend::CodeGenerator generator(emitter);

        // Create multiple literals with the same value
        auto literal1 = std::make_unique<LiteralExpression>(Int{42});
        auto literal2 = std::make_unique<LiteralExpression>(Int{42});
        auto literal3 = std::make_unique<LiteralExpression>(String{"hello"});
        auto literal4 = std::make_unique<LiteralExpression>(String{"hello"});

        generator.visit(*literal1);
        generator.visit(*literal2);
        generator.visit(*literal3);
        generator.visit(*literal4);

        auto function = emitter.get_function();

        // Print debug information
        std::cout << "Total constants: " << function.constants.size() << std::endl;
        for (size_t i = 0; i < function.constants.size(); ++i) {
            std::cout << "Constant " << i << ": " << constant_value_to_string(function.constants[i]) << std::endl;
        }

        // Check if deduplication is working
        // If deduplication works, we should have only 2 constants (42 and "hello")
        REQUIRE(function.constants.size() <= 2);
    }

    SECTION("Test constant deduplication with different types") {
        backend::BytecodeEmitter emitter("test_dedup_types");
        backend::CodeGenerator generator(emitter);

        // Create literals with different types but same numeric value
        auto int_literal = std::make_unique<LiteralExpression>(Int{42});        // Uses OP_LOADI (not in constants)
        auto number_literal = std::make_unique<LiteralExpression>(Number{42.0}); // Goes to constants
        auto bool_literal = std::make_unique<LiteralExpression>(true);          // Uses OP_LOADTRUE (not in constants)
        auto string_literal = std::make_unique<LiteralExpression>(String{"42"}); // Goes to constants

        generator.visit(*int_literal);
        generator.visit(*number_literal);
        generator.visit(*bool_literal);
        generator.visit(*string_literal);

        auto function = emitter.get_function();

        // Print debug information
        // std::cout << "Total constants (different types): " << function.constants.size() << std::endl;
        // for (size_t i = 0; i < function.constants.size(); ++i) {
        //     std::cout << "Constant " << i << ": " << constant_value_to_string(function.constants[i]) << std::endl;
        // }

        // Only Number and String go to constants (Int uses OP_LOADI, bool uses OP_LOADTRUE)
        REQUIRE(function.constants.size() == 2);
    }

    SECTION("Test large integer constant deduplication") {
        backend::BytecodeEmitter emitter("test_large_int_dedup");
        backend::CodeGenerator generator(emitter);

        // Create large integers that exceed OP_LOADI range (-32768 to 32767)
        auto large_int1 = std::make_unique<LiteralExpression>(Int{100000});
        auto large_int2 = std::make_unique<LiteralExpression>(Int{100000}); // Same value
        auto large_int3 = std::make_unique<LiteralExpression>(Int{200000}); // Different value

        generator.visit(*large_int1);
        generator.visit(*large_int2);
        generator.visit(*large_int3);

        auto function = emitter.get_function();

        // Print debug information
        // std::cout << "Total constants (large integers): " << function.constants.size() << std::endl;
        // for (size_t i = 0; i < function.constants.size(); ++i) {
        //     std::cout << "Constant " << i << ": " << constant_value_to_string(function.constants[i]) << std::endl;
        // }

        // Large integers go to constants and should be deduplicated
        REQUIRE(function.constants.size() == 2); // 100000 and 200000
    }
}
