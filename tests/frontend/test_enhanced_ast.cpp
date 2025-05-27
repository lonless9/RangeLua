/**
 * @file test_enhanced_ast.cpp
 * @brief Test cases for the enhanced AST implementation
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

#include <rangelua/frontend/ast.hpp>
#include <rangelua/frontend/parser.hpp>

using namespace rangelua;
using namespace rangelua::frontend;

TEST_CASE("Enhanced AST Node Types", "[ast][enhanced]") {
    SECTION("Binary operators include all Lua 5.5 operators") {
        // Test that all Lua 5.5 binary operators are supported
        auto left = ASTBuilder::make_literal(Int{1});
        auto right = ASTBuilder::make_literal(Int{2});

        // Arithmetic operators
        auto add = ASTBuilder::make_binary_op(BinaryOpExpression::Operator::Add,
                                             std::move(left), std::move(right));
        REQUIRE(add != nullptr);
        REQUIRE(add->type() == ASTNode::NodeType::BinaryOp);

        // Test integer division (Lua 5.3+)
        left = ASTBuilder::make_literal(Int{10});
        right = ASTBuilder::make_literal(Int{3});
        auto idiv = ASTBuilder::make_binary_op(BinaryOpExpression::Operator::IntegerDivide,
                                              std::move(left), std::move(right));
        REQUIRE(idiv != nullptr);

        // Test bitwise operators (Lua 5.3+)
        left = ASTBuilder::make_literal(Int{5});
        right = ASTBuilder::make_literal(Int{3});
        auto band = ASTBuilder::make_binary_op(BinaryOpExpression::Operator::BitwiseAnd,
                                              std::move(left), std::move(right));
        REQUIRE(band != nullptr);
    }

    SECTION("Unary operators include all Lua 5.5 operators") {
        auto operand = ASTBuilder::make_literal(Int{42});

        // Test bitwise not (Lua 5.3+)
        auto bnot = ASTBuilder::make_unary_op(UnaryOpExpression::Operator::BitwiseNot,
                                             std::move(operand));
        REQUIRE(bnot != nullptr);
        REQUIRE(bnot->type() == ASTNode::NodeType::UnaryOp);

        // Test length operator
        operand = ASTBuilder::make_literal(String{"hello"});
        auto len = ASTBuilder::make_unary_op(UnaryOpExpression::Operator::Length,
                                            std::move(operand));
        REQUIRE(len != nullptr);
    }
}

TEST_CASE("Table-related AST Nodes", "[ast][table]") {
    SECTION("Table access expression") {
        auto table = ASTBuilder::make_identifier("myTable");
        auto key = ASTBuilder::make_literal(String{"key"});

        // Bracket notation: myTable["key"]
        auto bracket_access = ASTBuilder::make_table_access(std::move(table), std::move(key), false);
        REQUIRE(bracket_access != nullptr);
        REQUIRE(bracket_access->type() == ASTNode::NodeType::TableAccess);

        // Dot notation: myTable.field
        table = ASTBuilder::make_identifier("myTable");
        key = ASTBuilder::make_literal(String{"field"});
        auto dot_access = ASTBuilder::make_table_access(std::move(table), std::move(key), true);
        REQUIRE(dot_access != nullptr);
    }

    SECTION("Table constructor expression") {
        TableConstructorExpression::FieldList fields;

        // Array field: [1] = "value"
        auto key = ASTBuilder::make_literal(Int{1});
        auto value = ASTBuilder::make_literal(String{"value"});
        fields.emplace_back(TableConstructorExpression::Field::Type::Array,
                           std::move(key), std::move(value));

        // Record field: key = "value"
        key = ASTBuilder::make_literal(String{"name"});
        value = ASTBuilder::make_literal(String{"John"});
        fields.emplace_back(TableConstructorExpression::Field::Type::Record,
                           std::move(key), std::move(value));

        // List field: "item"
        value = ASTBuilder::make_literal(String{"item"});
        fields.emplace_back(TableConstructorExpression::Field::Type::List,
                           nullptr, std::move(value));

        auto table_constructor = ASTBuilder::make_table_constructor(std::move(fields));
        REQUIRE(table_constructor != nullptr);
        REQUIRE(table_constructor->type() == ASTNode::NodeType::TableConstructor);
    }

    SECTION("Method call expression") {
        auto object = ASTBuilder::make_identifier("obj");
        ExpressionList args;
        args.push_back(ASTBuilder::make_literal(String{"arg1"}));
        args.push_back(ASTBuilder::make_literal(Int{42}));

        auto method_call = ASTBuilder::make_method_call(std::move(object), "method", std::move(args));
        REQUIRE(method_call != nullptr);
        REQUIRE(method_call->type() == ASTNode::NodeType::MethodCall);
    }
}

TEST_CASE("Function-related AST Nodes", "[ast][function]") {
    SECTION("Function expression") {
        FunctionExpression::ParameterList params;
        params.emplace_back("x", false);
        params.emplace_back("y", false);
        params.emplace_back("...", true);  // vararg parameter

        StatementList body_stmts;
        auto return_expr = ASTBuilder::make_binary_op(BinaryOpExpression::Operator::Add,
                                                     ASTBuilder::make_identifier("x"),
                                                     ASTBuilder::make_identifier("y"));
        ExpressionList return_values;
        return_values.push_back(std::move(return_expr));
        body_stmts.push_back(ASTBuilder::make_return(std::move(return_values)));

        auto body = ASTBuilder::make_block(std::move(body_stmts));
        auto func_expr = ASTBuilder::make_function_expression(std::move(params), std::move(body));

        REQUIRE(func_expr != nullptr);
        REQUIRE(func_expr->type() == ASTNode::NodeType::FunctionExpression);
    }

    SECTION("Vararg expression") {
        auto vararg = ASTBuilder::make_vararg();
        REQUIRE(vararg != nullptr);
        REQUIRE(vararg->type() == ASTNode::NodeType::Vararg);
    }
}

TEST_CASE("Statement AST Nodes", "[ast][statement]") {
    SECTION("Local declaration statement") {
        std::vector<String> names = {"x", "y", "z"};
        ExpressionList values;
        values.push_back(ASTBuilder::make_literal(Int{1}));
        values.push_back(ASTBuilder::make_literal(Int{2}));
        values.push_back(ASTBuilder::make_literal(Int{3}));

        auto local_decl = ASTBuilder::make_local_declaration(std::move(names), std::move(values));
        REQUIRE(local_decl != nullptr);
        REQUIRE(local_decl->type() == ASTNode::NodeType::LocalDeclaration);
    }

    SECTION("While statement") {
        auto condition = ASTBuilder::make_binary_op(BinaryOpExpression::Operator::Less,
                                                   ASTBuilder::make_identifier("i"),
                                                   ASTBuilder::make_literal(Int{10}));

        StatementList body_stmts;
        auto increment = ASTBuilder::make_binary_op(BinaryOpExpression::Operator::Add,
                                                   ASTBuilder::make_identifier("i"),
                                                   ASTBuilder::make_literal(Int{1}));
        ExpressionList targets;
        targets.push_back(ASTBuilder::make_identifier("i"));
        ExpressionList values;
        values.push_back(std::move(increment));
        body_stmts.push_back(ASTBuilder::make_assignment(std::move(targets), std::move(values)));

        auto body = ASTBuilder::make_block(std::move(body_stmts));
        auto while_stmt = ASTBuilder::make_while(std::move(condition), std::move(body));

        REQUIRE(while_stmt != nullptr);
        REQUIRE(while_stmt->type() == ASTNode::NodeType::WhileStatement);
    }

    SECTION("For numeric statement") {
        auto start = ASTBuilder::make_literal(Int{1});
        auto stop = ASTBuilder::make_literal(Int{10});
        auto step = ASTBuilder::make_literal(Int{1});
        auto body = ASTBuilder::make_block(StatementList{});

        auto for_stmt = ASTBuilder::make_for_numeric("i", std::move(start), std::move(stop),
                                                    std::move(step), std::move(body));

        REQUIRE(for_stmt != nullptr);
        REQUIRE(for_stmt->type() == ASTNode::NodeType::ForNumericStatement);
    }

    SECTION("For generic statement") {
        std::vector<String> variables = {"k", "v"};
        ExpressionList expressions;

        // Create the pairs function call arguments
        ExpressionList pairs_args;
        pairs_args.push_back(ASTBuilder::make_identifier("table"));

        expressions.push_back(ASTBuilder::make_function_call(ASTBuilder::make_identifier("pairs"),
                                                            std::move(pairs_args)));
        auto body = ASTBuilder::make_block(StatementList{});

        auto for_stmt = ASTBuilder::make_for_generic(std::move(variables), std::move(expressions),
                                                    std::move(body));

        REQUIRE(for_stmt != nullptr);
        REQUIRE(for_stmt->type() == ASTNode::NodeType::ForGenericStatement);
    }
}

TEST_CASE("Control Flow AST Nodes", "[ast][control]") {
    SECTION("Goto and label statements") {
        auto goto_stmt = ASTBuilder::make_goto("loop_start");
        REQUIRE(goto_stmt != nullptr);
        REQUIRE(goto_stmt->type() == ASTNode::NodeType::GotoStatement);

        auto label_stmt = ASTBuilder::make_label("loop_start");
        REQUIRE(label_stmt != nullptr);
        REQUIRE(label_stmt->type() == ASTNode::NodeType::LabelStatement);
    }

    SECTION("Break statement") {
        auto break_stmt = ASTBuilder::make_break();
        REQUIRE(break_stmt != nullptr);
        REQUIRE(break_stmt->type() == ASTNode::NodeType::BreakStatement);
    }

    SECTION("Return statement") {
        ExpressionList values;
        values.push_back(ASTBuilder::make_literal(Int{42}));
        values.push_back(ASTBuilder::make_literal(String{"success"}));

        auto return_stmt = ASTBuilder::make_return(std::move(values));
        REQUIRE(return_stmt != nullptr);
        REQUIRE(return_stmt->type() == ASTNode::NodeType::ReturnStatement);
    }
}

TEST_CASE("AST Visitor Pattern", "[ast][visitor]") {
    SECTION("Visitor pattern works with new nodes") {
        // Create a simple visitor that counts nodes
        class NodeCounter : public ASTVisitor {
        public:
            int count = 0;

            void visit(const LiteralExpression&) override { count++; }
            void visit(const IdentifierExpression&) override { count++; }
            void visit(const BinaryOpExpression&) override { count++; }
            void visit(const UnaryOpExpression&) override { count++; }
            void visit(const FunctionCallExpression&) override { count++; }
            void visit(const MethodCallExpression&) override { count++; }
            void visit(const TableAccessExpression&) override { count++; }
            void visit(const TableConstructorExpression&) override { count++; }
            void visit(const FunctionExpression&) override { count++; }
            void visit(const VarargExpression&) override { count++; }
            void visit(const ParenthesizedExpression&) override { count++; }
            void visit(const BlockStatement&) override { count++; }
            void visit(const AssignmentStatement&) override { count++; }
            void visit(const LocalDeclarationStatement&) override { count++; }
            void visit(const FunctionDeclarationStatement&) override { count++; }
            void visit(const IfStatement&) override { count++; }
            void visit(const WhileStatement&) override { count++; }
            void visit(const ForNumericStatement&) override { count++; }
            void visit(const ForGenericStatement&) override { count++; }
            void visit(const RepeatStatement&) override { count++; }
            void visit(const DoStatement&) override { count++; }
            void visit(const ReturnStatement&) override { count++; }
            void visit(const BreakStatement&) override { count++; }
            void visit(const GotoStatement&) override { count++; }
            void visit(const LabelStatement&) override { count++; }
            void visit(const ExpressionStatement&) override { count++; }
            void visit(const Program&) override { count++; }
        };

        NodeCounter counter;

        // Test with a method call
        auto method_call = ASTBuilder::make_method_call(
            ASTBuilder::make_identifier("obj"),
            "method",
            ExpressionList{}
        );
        method_call->accept(counter);
        REQUIRE(counter.count == 1);

        // Test with a table constructor
        counter.count = 0;
        auto table_constructor = ASTBuilder::make_table_constructor(TableConstructorExpression::FieldList{});
        table_constructor->accept(counter);
        REQUIRE(counter.count == 1);
    }
}
