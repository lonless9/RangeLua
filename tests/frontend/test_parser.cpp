/**
 * @file test_parser.cpp
 * @brief Parser tests
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/frontend/parser.hpp>
#include <rangelua/frontend/lexer.hpp>

using namespace rangelua::frontend;

TEST_CASE("Parser basic functionality", "[frontend][parser]") {
    SECTION("Parse empty program") {
        Lexer lexer("", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse();
        REQUIRE(is_success(result));
        
        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().empty());
    }
    
    SECTION("Parse simple literal expressions") {
        Lexer lexer("42", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse_expression();
        REQUIRE(is_success(result));
        
        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == ASTNode::NodeType::Literal);
    }
    
    SECTION("Parse identifier") {
        Lexer lexer("x", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse_expression();
        REQUIRE(is_success(result));
        
        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == ASTNode::NodeType::Identifier);
    }
    
    SECTION("Parse binary expression") {
        Lexer lexer("1 + 2", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse_expression();
        REQUIRE(is_success(result));
        
        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == ASTNode::NodeType::BinaryOp);
    }
    
    SECTION("Parse local variable declaration") {
        Lexer lexer("local x = 42", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse();
        REQUIRE(is_success(result));
        
        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == ASTNode::NodeType::LocalDeclaration);
    }
    
    SECTION("Parse return statement") {
        Lexer lexer("return 42", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse();
        REQUIRE(is_success(result));
        
        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == ASTNode::NodeType::ReturnStatement);
    }
}

TEST_CASE("Parser error handling", "[frontend][parser]") {
    SECTION("Invalid syntax produces error") {
        Lexer lexer("local = 42", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse();
        // Should still return a program but with errors
        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());
    }
    
    SECTION("Unexpected token in expression") {
        Lexer lexer("1 + +", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse_expression();
        REQUIRE(!is_success(result));
    }
}

TEST_CASE("Parser precedence", "[frontend][parser]") {
    SECTION("Arithmetic precedence") {
        Lexer lexer("1 + 2 * 3", "test.lua");
        Parser parser(lexer);
        
        auto result = parser.parse_expression();
        REQUIRE(is_success(result));
        
        auto expr = get_value(std::move(result));
        REQUIRE(expr != nullptr);
        REQUIRE(expr->type() == ASTNode::NodeType::BinaryOp);
        
        // Should parse as (1 + (2 * 3))
        auto binary_expr = static_cast<const BinaryOpExpression*>(expr.get());
        REQUIRE(binary_expr->operator_type() == BinaryOpExpression::Operator::Add);
        REQUIRE(binary_expr->right().type() == ASTNode::NodeType::BinaryOp);
    }
}
