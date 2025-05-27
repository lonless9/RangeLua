/**
 * @file test_parser.cpp
 * @brief Comprehensive Parser tests for Lua 5.5 syntax
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/frontend/parser.hpp>
#include <rangelua/frontend/lexer.hpp>

using namespace rangelua;

TEST_CASE("Parser basic functionality", "[frontend][parser]") {
    SECTION("Parse empty program") {
        frontend::Lexer lexer("", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().empty());
    }

    SECTION("Parse simple literal expressions") {
        frontend::Lexer lexer("42", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::Literal);
    }

    SECTION("Parse identifier") {
        frontend::Lexer lexer("x", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::Identifier);
    }

    SECTION("Parse binary expression") {
        frontend::Lexer lexer("1 + 2", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::BinaryOp);
    }

    SECTION("Parse local variable declaration") {
        frontend::Lexer lexer("local x = 42", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == frontend::ASTNode::NodeType::LocalDeclaration);
    }

    SECTION("Parse return statement") {
        frontend::Lexer lexer("return 42", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == frontend::ASTNode::NodeType::ReturnStatement);
    }
}

TEST_CASE("Parser error handling", "[frontend][parser]") {
    SECTION("Invalid syntax produces error") {
        frontend::Lexer lexer("local = 42", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        // Should still return a program but with errors
        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());
    }

    SECTION("Unexpected token in expression") {
        frontend::Lexer lexer("1 + +", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(!is_success(result));
    }
}

TEST_CASE("Parser precedence", "[frontend][parser]") {
    SECTION("Arithmetic precedence") {
        frontend::Lexer lexer("1 + 2 * 3", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::BinaryOp);
    }
}

// ============================================================================
// Comprehensive Lua 5.5 Syntax Tests
// ============================================================================

TEST_CASE("Literal expressions", "[frontend][parser][literals]") {
    SECTION("Integer literals") {
        frontend::Lexer lexer("123", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::Literal);
    }

    SECTION("String literals") {
        frontend::Lexer lexer("\"hello world\"", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::Literal);
    }

    SECTION("Boolean literals") {
        frontend::Lexer lexer("true", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::Literal);
    }

    SECTION("Nil literal") {
        frontend::Lexer lexer("nil", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::Literal);
    }
}

TEST_CASE("Binary operators", "[frontend][parser][operators]") {
    SECTION("Arithmetic operators") {
        frontend::Lexer lexer("1 + 2", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::BinaryOp);
    }

    SECTION("Comparison operators") {
        frontend::Lexer lexer("a == b", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::BinaryOp);
    }

    SECTION("Logical operators") {
        frontend::Lexer lexer("a and b", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::BinaryOp);
    }
}

TEST_CASE("Control structures", "[frontend][parser][control]") {
    SECTION("If statement") {
        frontend::Lexer lexer("if x then y = 1 end", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == frontend::ASTNode::NodeType::IfStatement);
    }

    SECTION("While loop") {
        frontend::Lexer lexer("while x < 10 do x = x + 1 end", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == frontend::ASTNode::NodeType::WhileStatement);
    }

    SECTION("For loop") {
        frontend::Lexer lexer("for i = 1, 10 do print(i) end", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() ==
                frontend::ASTNode::NodeType::ForNumericStatement);
    }
}

TEST_CASE("Function declarations", "[frontend][parser][functions]") {
    SECTION("Simple function") {
        frontend::Lexer lexer("function foo() return 42 end", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() ==
                frontend::ASTNode::NodeType::FunctionDeclaration);
    }

    SECTION("Function with parameters") {
        frontend::Lexer lexer("function add(x, y) return x + y end", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() ==
                frontend::ASTNode::NodeType::FunctionDeclaration);
    }
}

TEST_CASE("Table operations", "[frontend][parser][tables]") {
    SECTION("Table constructor") {
        frontend::Lexer lexer("{1, 2, 3}", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::TableConstructor);
    }

    SECTION("Table access") {
        frontend::Lexer lexer("table[key]", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::TableAccess);
    }
}

TEST_CASE("Complex expressions", "[frontend][parser][complex]") {
    SECTION("Nested expressions") {
        frontend::Lexer lexer("(1 + 2) * (3 - 4)", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::BinaryOp);
    }

    SECTION("Function calls") {
        frontend::Lexer lexer("func(1, 2, 3)", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::FunctionCall);
    }

    SECTION("Method calls") {
        frontend::Lexer lexer("obj:method()", "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse_expression();
        REQUIRE(is_success(result));

        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == frontend::ASTNode::NodeType::MethodCall);
    }
}

TEST_CASE("Integration tests", "[frontend][parser][integration]") {
    SECTION("Simple Lua program") {
        const String lua_code = R"(
            local x = 10
            local y = 20
            if x < y then
                return y
            else
                return x
            end
        )";

        frontend::Lexer lexer(lua_code, "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 3);  // 2 locals + 1 if statement
    }

    SECTION("Function definition") {
        const String lua_code = "function add(a, b) return a + b end";

        frontend::Lexer lexer(lua_code, "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() ==
                frontend::ASTNode::NodeType::FunctionDeclaration);
    }

    SECTION("Table operations") {
        const String lua_code = "local t = {x = 1, y = 2}; return t.x + t.y";

        frontend::Lexer lexer(lua_code, "test.lua");
        frontend::Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 2);  // local + return
    }
}
