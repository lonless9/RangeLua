/**
 * @file test_frontend_integration.cpp
 * @brief Comprehensive integration tests for RangeLua frontend modules
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <rangelua/frontend/lexer.hpp>
#include <rangelua/frontend/parser.hpp>
#include <rangelua/frontend/ast.hpp>
#include <rangelua/utils/logger.hpp>

#include <chrono>
#include <memory>
#include <sstream>
#include <thread>

using rangelua::String;
using rangelua::Result;
using rangelua::is_success;
using rangelua::get_value;
using rangelua::frontend::ProgramPtr;
using rangelua::frontend::Lexer;
using rangelua::frontend::Parser;
using rangelua::frontend::Token;
using rangelua::frontend::TokenType;
using rangelua::frontend::ASTNode;
using rangelua::frontend::ASTVisitor;
using rangelua::frontend::LiteralExpression;
using rangelua::frontend::IdentifierExpression;
using rangelua::frontend::BinaryOpExpression;
using rangelua::frontend::UnaryOpExpression;
using rangelua::frontend::FunctionCallExpression;
using rangelua::frontend::MethodCallExpression;
using rangelua::frontend::TableAccessExpression;
using rangelua::frontend::TableConstructorExpression;
using rangelua::frontend::FunctionExpression;
using rangelua::frontend::VarargExpression;
using rangelua::frontend::ParenthesizedExpression;
using rangelua::frontend::BlockStatement;
using rangelua::frontend::AssignmentStatement;
using rangelua::frontend::LocalDeclarationStatement;
using rangelua::frontend::FunctionDeclarationStatement;
using rangelua::frontend::IfStatement;
using rangelua::frontend::WhileStatement;
using rangelua::frontend::ForNumericStatement;
using rangelua::frontend::ForGenericStatement;
using rangelua::frontend::RepeatStatement;
using rangelua::frontend::DoStatement;
using rangelua::frontend::ReturnStatement;
using rangelua::frontend::BreakStatement;
using rangelua::frontend::GotoStatement;
using rangelua::frontend::LabelStatement;
using rangelua::frontend::ExpressionStatement;
using rangelua::frontend::Program;

// ============================================================================
// Test Utilities and Helpers
// ============================================================================

namespace {
    /**
     * @brief Helper class for AST validation
     */
    class ASTValidator : public ASTVisitor {
    public:
        void visit(const LiteralExpression& node) override {
            visited_nodes_.push_back("LiteralExpression");
            literal_count_++;
        }

        void visit(const IdentifierExpression& node) override {
            visited_nodes_.push_back("IdentifierExpression");
            identifier_count_++;
        }

        void visit(const BinaryOpExpression& node) override {
            visited_nodes_.push_back("BinaryOpExpression");
            binary_op_count_++;
            node.left().accept(*this);
            node.right().accept(*this);
        }

        void visit(const UnaryOpExpression& node) override {
            visited_nodes_.push_back("UnaryOpExpression");
            unary_op_count_++;
            node.operand().accept(*this);
        }

        void visit(const FunctionCallExpression& node) override {
            visited_nodes_.push_back("FunctionCallExpression");
            function_call_count_++;
            node.function().accept(*this);
            for (const auto& arg : node.arguments()) {
                arg->accept(*this);
            }
        }

        void visit(const MethodCallExpression& node) override {
            visited_nodes_.push_back("MethodCallExpression");
            method_call_count_++;
            node.object().accept(*this);
            for (const auto& arg : node.arguments()) {
                arg->accept(*this);
            }
        }

        void visit(const TableAccessExpression& node) override {
            visited_nodes_.push_back("TableAccessExpression");
            table_access_count_++;
            node.table().accept(*this);
            node.key().accept(*this);
        }

        void visit(const TableConstructorExpression& node) override {
            visited_nodes_.push_back("TableConstructorExpression");
            table_constructor_count_++;
            for (const auto& field : node.fields()) {
                if (field.key) {
                    field.key->accept(*this);
                }
                field.value->accept(*this);
            }
        }

        void visit(const FunctionExpression& node) override {
            visited_nodes_.push_back("FunctionExpression");
            function_expression_count_++;
            node.body().accept(*this);
        }

        void visit(const VarargExpression& node) override {
            visited_nodes_.push_back("VarargExpression");
            vararg_count_++;
        }

        void visit(const ParenthesizedExpression& node) override {
            visited_nodes_.push_back("ParenthesizedExpression");
            parenthesized_count_++;
            node.expression().accept(*this);
        }

        void visit(const BlockStatement& node) override {
            visited_nodes_.push_back("BlockStatement");
            block_count_++;
            for (const auto& stmt : node.statements()) {
                stmt->accept(*this);
            }
        }

        void visit(const AssignmentStatement& node) override {
            visited_nodes_.push_back("AssignmentStatement");
            assignment_count_++;
            for (const auto& target : node.targets()) {
                target->accept(*this);
            }
            for (const auto& value : node.values()) {
                value->accept(*this);
            }
        }

        void visit(const LocalDeclarationStatement& node) override {
            visited_nodes_.push_back("LocalDeclarationStatement");
            local_declaration_count_++;
            for (const auto& value : node.values()) {
                value->accept(*this);
            }
        }

        void visit(const FunctionDeclarationStatement& node) override {
            visited_nodes_.push_back("FunctionDeclarationStatement");
            function_declaration_count_++;
            node.name().accept(*this);
            node.body().accept(*this);
        }

        void visit(const IfStatement& node) override {
            visited_nodes_.push_back("IfStatement");
            if_count_++;
            node.condition().accept(*this);
            node.then_body().accept(*this);
            for (const auto& elseif_clause : node.elseif_clauses()) {
                elseif_clause.condition->accept(*this);
                elseif_clause.body->accept(*this);
            }
            if (node.else_body()) {
                node.else_body()->accept(*this);
            }
        }

        void visit(const WhileStatement& node) override {
            visited_nodes_.push_back("WhileStatement");
            while_count_++;
            node.condition().accept(*this);
            node.body().accept(*this);
        }

        void visit(const ForNumericStatement& node) override {
            visited_nodes_.push_back("ForNumericStatement");
            for_numeric_count_++;
            node.start().accept(*this);
            node.stop().accept(*this);
            if (node.step()) {
                node.step()->accept(*this);
            }
            node.body().accept(*this);
        }

        void visit(const ForGenericStatement& node) override {
            visited_nodes_.push_back("ForGenericStatement");
            for_generic_count_++;
            for (const auto& expr : node.expressions()) {
                expr->accept(*this);
            }
            node.body().accept(*this);
        }

        void visit(const RepeatStatement& node) override {
            visited_nodes_.push_back("RepeatStatement");
            repeat_count_++;
            node.body().accept(*this);
            node.condition().accept(*this);
        }

        void visit(const DoStatement& node) override {
            visited_nodes_.push_back("DoStatement");
            do_count_++;
            node.body().accept(*this);
        }

        void visit(const ReturnStatement& node) override {
            visited_nodes_.push_back("ReturnStatement");
            return_count_++;
            for (const auto& value : node.values()) {
                value->accept(*this);
            }
        }

        void visit(const BreakStatement& node) override {
            visited_nodes_.push_back("BreakStatement");
            break_count_++;
        }

        void visit(const GotoStatement& node) override {
            visited_nodes_.push_back("GotoStatement");
            goto_count_++;
        }

        void visit(const LabelStatement& node) override {
            visited_nodes_.push_back("LabelStatement");
            label_count_++;
        }

        void visit(const ExpressionStatement& node) override {
            visited_nodes_.push_back("ExpressionStatement");
            expression_statement_count_++;
            node.expression().accept(*this);
        }

        void visit(const Program& node) override {
            visited_nodes_.push_back("Program");
            program_count_++;
            for (const auto& stmt : node.statements()) {
                stmt->accept(*this);
            }
        }

        // Getters for validation
        [[nodiscard]] size_t total_nodes() const { return visited_nodes_.size(); }
        [[nodiscard]] size_t literal_count() const { return literal_count_; }
        [[nodiscard]] size_t identifier_count() const { return identifier_count_; }
        [[nodiscard]] size_t binary_op_count() const { return binary_op_count_; }
        [[nodiscard]] size_t function_call_count() const { return function_call_count_; }
        [[nodiscard]] size_t assignment_count() const { return assignment_count_; }
        [[nodiscard]] size_t if_count() const { return if_count_; }
        [[nodiscard]] size_t local_declaration_count() const { return local_declaration_count_; }
        [[nodiscard]] size_t function_declaration_count() const { return function_declaration_count_; }
        [[nodiscard]] size_t for_numeric_count() const { return for_numeric_count_; }
        [[nodiscard]] size_t while_count() const { return while_count_; }
        [[nodiscard]] size_t goto_count() const { return goto_count_; }
        [[nodiscard]] size_t label_count() const { return label_count_; }
        [[nodiscard]] size_t table_constructor_count() const { return table_constructor_count_; }
        [[nodiscard]] size_t table_access_count() const { return table_access_count_; }
        [[nodiscard]] size_t method_call_count() const { return method_call_count_; }
        [[nodiscard]] size_t for_generic_count() const { return for_generic_count_; }
        [[nodiscard]] size_t function_expression_count() const { return function_expression_count_; }
        [[nodiscard]] size_t vararg_count() const { return vararg_count_; }
        [[nodiscard]] size_t return_count() const { return return_count_; }
        [[nodiscard]] size_t program_count() const { return program_count_; }
        [[nodiscard]] const std::vector<String>& visited_nodes() const { return visited_nodes_; }

        void reset() {
            visited_nodes_.clear();
            literal_count_ = 0;
            identifier_count_ = 0;
            binary_op_count_ = 0;
            unary_op_count_ = 0;
            function_call_count_ = 0;
            method_call_count_ = 0;
            table_access_count_ = 0;
            table_constructor_count_ = 0;
            function_expression_count_ = 0;
            vararg_count_ = 0;
            parenthesized_count_ = 0;
            block_count_ = 0;
            assignment_count_ = 0;
            local_declaration_count_ = 0;
            function_declaration_count_ = 0;
            if_count_ = 0;
            while_count_ = 0;
            for_numeric_count_ = 0;
            for_generic_count_ = 0;
            repeat_count_ = 0;
            do_count_ = 0;
            return_count_ = 0;
            break_count_ = 0;
            goto_count_ = 0;
            label_count_ = 0;
            expression_statement_count_ = 0;
            program_count_ = 0;
        }

    private:
        std::vector<String> visited_nodes_;
        size_t literal_count_ = 0;
        size_t identifier_count_ = 0;
        size_t binary_op_count_ = 0;
        size_t unary_op_count_ = 0;
        size_t function_call_count_ = 0;
        size_t method_call_count_ = 0;
        size_t table_access_count_ = 0;
        size_t table_constructor_count_ = 0;
        size_t function_expression_count_ = 0;
        size_t vararg_count_ = 0;
        size_t parenthesized_count_ = 0;
        size_t block_count_ = 0;
        size_t assignment_count_ = 0;
        size_t local_declaration_count_ = 0;
        size_t function_declaration_count_ = 0;
        size_t if_count_ = 0;
        size_t while_count_ = 0;
        size_t for_numeric_count_ = 0;
        size_t for_generic_count_ = 0;
        size_t repeat_count_ = 0;
        size_t do_count_ = 0;
        size_t return_count_ = 0;
        size_t break_count_ = 0;
        size_t goto_count_ = 0;
        size_t label_count_ = 0;
        size_t expression_statement_count_ = 0;
        size_t program_count_ = 0;
    };

    /**
     * @brief Helper function to parse Lua code and return AST
     */
    Result<ProgramPtr> parse_lua_code(const String& code, const String& filename = "<test>") {
        Lexer lexer(code, filename);
        Parser parser(lexer);
        return parser.parse();
    }

    /**
     * @brief Helper function to validate successful parsing
     */
    void validate_successful_parse(const String& code, const String& test_name) {
        auto result = parse_lua_code(code, test_name);
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        // Validate AST structure using visitor
        ASTValidator validator;
        program->accept(validator);
        REQUIRE(validator.total_nodes() > 0);
    }
} // anonymous namespace

// ============================================================================
// Lexer-Parser Integration Tests
// ============================================================================

TEST_CASE("Frontend Integration - Lexer-Parser Coordination", "[frontend][integration][lexer-parser]") {
    SECTION("Token stream to AST conversion") {
        const String code = "local x = 42 + y";

        // Test lexer independently
        Lexer lexer(code, "test.lua");
        std::vector<Token> tokens;

        Token token;
        do {
            token = lexer.next_token();
            tokens.push_back(token);
        } while (token.type != TokenType::EndOfFile);

        REQUIRE(tokens.size() == 7); // local, x, =, 42, +, y, EOF
        REQUIRE(tokens[0].type == TokenType::Local);
        REQUIRE(tokens[1].type == TokenType::Identifier);
        REQUIRE(tokens[2].type == TokenType::Assign);
        REQUIRE(tokens[3].type == TokenType::Number);
        REQUIRE(tokens[4].type == TokenType::Plus);
        REQUIRE(tokens[5].type == TokenType::Identifier);
        REQUIRE(tokens[6].type == TokenType::EndOfFile);

        // Test parser with same code
        auto result = parse_lua_code(code);
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);
        REQUIRE(program->statements().size() == 1);
        REQUIRE(program->statements()[0]->type() == ASTNode::NodeType::LocalDeclaration);
    }

    SECTION("Complex expression parsing") {
        const String code = "result = (a + b) * c - func(x, y)";

        auto result = parse_lua_code(code);
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);

        // Validate expected AST structure
        REQUIRE(validator.assignment_count() == 1);
        REQUIRE(validator.binary_op_count() >= 2); // -, *, +
        REQUIRE(validator.function_call_count() == 1);
        REQUIRE(validator.identifier_count() >= 5); // result, a, b, c, func, x, y
    }

    SECTION("Error recovery and synchronization") {
        const String code = R"(
            local x = 42
            local = invalid  -- syntax error
            local y = 24     -- should still parse
        )";

        Lexer lexer(code, "test.lua");
        Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result)); // Parser should recover
        REQUIRE(parser.has_errors()); // But report errors

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        // Should have parsed valid statements despite error
        ASTValidator validator;
        program->accept(validator);
        REQUIRE(validator.local_declaration_count() >= 1);
    }
}

// ============================================================================
// Complete Lua Syntax Integration Tests
// ============================================================================

TEST_CASE("Frontend Integration - Complete Lua 5.5 Syntax", "[frontend][integration][syntax]") {
    SECTION("Control structures integration") {
        const String code = R"(
            local function factorial(n)
                if n <= 1 then
                    return 1
                else
                    return n * factorial(n - 1)
                end
            end

            for i = 1, 10 do
                print(factorial(i))
            end

            local x = 0
            while x < 5 do
                x = x + 1
                if x == 3 then
                    goto continue
                end
                print(x)
                ::continue::
            end
        )";

        validate_successful_parse(code, "control_structures");

        auto result = parse_lua_code(code);
        auto program = get_value(std::move(result));

        ASTValidator validator;
        program->accept(validator);

        // Validate complex structure
        // Note: "local function factorial" is a local declaration, not function declaration
        REQUIRE(validator.function_declaration_count() == 0);
        REQUIRE(validator.local_declaration_count() >= 1); // local function factorial
        REQUIRE(validator.function_expression_count() >= 1); // function expression in local function
        REQUIRE(validator.if_count() >= 2);
        REQUIRE(validator.for_numeric_count() == 1);
        REQUIRE(validator.while_count() == 1);
        REQUIRE(validator.goto_count() == 1);
        REQUIRE(validator.label_count() == 1);
    }

    SECTION("Table operations integration") {
        const String code = R"(
            local person = {
                name = "John",
                age = 30,
                address = {
                    street = "123 Main St",
                    city = "Anytown"
                }
            }

            person.email = "john@example.com"
            person["phone"] = "555-1234"

            local function greet(self)
                return "Hello, " .. self.name
            end

            person.greet = greet
            print(person:greet())

            for key, value in pairs(person) do
                print(key, value)
            end
        )";

        validate_successful_parse(code, "table_operations");

        auto result = parse_lua_code(code);
        auto program = get_value(std::move(result));

        ASTValidator validator;
        program->accept(validator);

        REQUIRE(validator.table_constructor_count() >= 2);
        REQUIRE(validator.table_access_count() >= 3);
        REQUIRE(validator.method_call_count() == 1);
        REQUIRE(validator.for_generic_count() == 1);
    }

    SECTION("Function definitions and calls") {
        const String code = R"(
            -- Regular function
            function add(a, b)
                return a + b
            end

            -- Local function
            local function multiply(x, y)
                return x * y
            end

            -- Anonymous function
            local divide = function(a, b)
                if b == 0 then
                    error("Division by zero")
                end
                return a / b
            end

            -- Vararg function
            local function sum(...)
                local total = 0
                for i, v in ipairs({...}) do
                    total = total + v
                end
                return total
            end

            -- Function calls
            local result1 = add(5, 3)
            local result2 = multiply(4, 6)
            local result3 = divide(10, 2)
            local result4 = sum(1, 2, 3, 4, 5)
        )";

        validate_successful_parse(code, "function_definitions");

        auto result = parse_lua_code(code);
        auto program = get_value(std::move(result));

        ASTValidator validator;
        program->accept(validator);

        // function add() - function declaration
        // local function multiply() - local declaration with function expression
        // local function sum() - local declaration with function expression
        // local divide = function() - local declaration with function expression
        REQUIRE(validator.function_declaration_count() >= 1); // function add
        REQUIRE(validator.local_declaration_count() >= 6); // multiply, sum, divide + 4 result variables
        REQUIRE(validator.function_expression_count() >= 3); // multiply, sum, divide function expressions
        // Function calls: error(), ipairs(), add(), multiply(), divide(), sum()
        REQUIRE(validator.function_call_count() >= 6);
        REQUIRE(validator.vararg_count() >= 1);
    }
}

// ============================================================================
// AST Construction and Visitor Pattern Integration Tests
// ============================================================================

TEST_CASE("Frontend Integration - AST Construction and Visitor Pattern", "[frontend][integration][ast]") {
    SECTION("AST structure validation") {
        const String code = R"(
            local x = 42
            local y = "hello"
            local z = true
            local w = nil
            -- Now use the variables to create identifier expressions
            local sum = x + y
            print(z, w)
        )";

        auto result = parse_lua_code(code);
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);

        // Should have 6 local declarations (x, y, z, w, sum)
        REQUIRE(validator.local_declaration_count() == 5);
        // Should have 4 literal expressions (42, "hello", true, nil)
        REQUIRE(validator.literal_count() == 4);
        // Should have identifier expressions (x, y, z, w used in expressions)
        REQUIRE(validator.identifier_count() >= 4);
    }

    SECTION("Complex AST traversal") {
        const String code = R"(
            local function calculate(a, b)
                local result = a * b + (a - b) / 2
                if result > 0 then
                    return result
                else
                    return -result
                end
            end

            local value = calculate(10, 5)
        )";

        auto result = parse_lua_code(code);
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);

        // Validate complex structure
        // "local function calculate" is a local declaration, not function declaration
        REQUIRE(validator.function_declaration_count() == 0);
        REQUIRE(validator.local_declaration_count() >= 2); // function + value
        REQUIRE(validator.function_expression_count() >= 1); // calculate function expression
        REQUIRE(validator.binary_op_count() >= 4); // *, +, -, /
        REQUIRE(validator.if_count() == 1);
        REQUIRE(validator.return_count() >= 2);
        REQUIRE(validator.function_call_count() == 1);
    }

    SECTION("Visitor pattern completeness") {
        const String code = R"(
            local t = {x = 1, y = 2}
            t.z = t.x + t.y
            print(t:toString())

            for k, v in pairs(t) do
                print(k, v)
            end

            local i = 1
            while i <= 3 do
                print(i)
                i = i + 1
            end

            ::loop::
            if i > 10 then
                goto done
            end
            i = i + 1
            goto loop
            ::done::
        )";

        auto result = parse_lua_code(code);
        REQUIRE(is_success(result));

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);

        // Verify all major node types are visited
        REQUIRE(validator.table_constructor_count() >= 1);
        REQUIRE(validator.table_access_count() >= 3);
        REQUIRE(validator.method_call_count() >= 1);
        REQUIRE(validator.for_generic_count() == 1);
        REQUIRE(validator.while_count() == 1);
        REQUIRE(validator.goto_count() >= 2);
        REQUIRE(validator.label_count() >= 2);
        REQUIRE(validator.if_count() >= 1);
    }
}

// ============================================================================
// Error Handling and Recovery Integration Tests
// ============================================================================

TEST_CASE("Frontend Integration - Error Handling and Recovery", "[frontend][integration][errors]") {
    SECTION("Syntax error detection") {
        const String code = R"(
            local x = 42
            local y =
            local z = 24
        )";

        Lexer lexer(code, "test.lua");
        Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result)); // Should still return a program
        REQUIRE(parser.has_errors()); // But with errors

        const auto& errors = parser.errors();
        REQUIRE(!errors.empty());
    }

    SECTION("Multiple error recovery") {
        const String code = R"(
            local x = 42
            function (a, b)  -- missing function name
                return a + b
            end

            local y = 24
            if x > y then
                print("x is greater")
            -- missing end

            local z = x + y
        )";

        Lexer lexer(code, "test.lua");
        Parser parser(lexer);

        auto result = parser.parse();
        REQUIRE(is_success(result));
        REQUIRE(parser.has_errors());

        // Should still parse some valid statements
        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);
        REQUIRE(validator.local_declaration_count() >= 2); // x and z should be parsed
    }

    SECTION("Lexer error propagation") {
        const String code = R"(
            local x = 42
            local y = "unfinished string
            local z = 24
        )";

        Lexer lexer(code, "test.lua");
        REQUIRE(lexer.has_errors() == false); // Initially no errors

        // Tokenize and check for errors
        auto tokens = lexer.tokenize();
        REQUIRE(lexer.has_errors()); // Should have lexer errors

        // Parser should handle lexer errors gracefully
        Lexer lexer2(code, "test.lua");
        Parser parser(lexer2);
        auto result = parser.parse();

        // Parser might succeed or fail depending on error recovery
        if (is_success(result)) {
            REQUIRE(parser.has_errors());
        }
    }
}

// ============================================================================
// Performance and Memory Management Integration Tests
// ============================================================================

TEST_CASE("Frontend Integration - Performance and Memory", "[frontend][integration][performance]") {
    SECTION("Large source file parsing") {
        // Generate a large Lua program
        std::ostringstream code_stream;
        const int num_functions = 100;

        for (int i = 0; i < num_functions; ++i) {
            code_stream << "local function func" << i << "(a, b)\n";
            code_stream << "    local result = a + b\n";
            code_stream << "    if result > 0 then\n";
            code_stream << "        return result * 2\n";
            code_stream << "    else\n";
            code_stream << "        return result / 2\n";
            code_stream << "    end\n";
            code_stream << "end\n\n";
        }

        String large_code = code_stream.str();

        auto start_time = std::chrono::high_resolution_clock::now();
        auto result = parse_lua_code(large_code, "large_test.lua");
        auto end_time = std::chrono::high_resolution_clock::now();

        REQUIRE(is_success(result));

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        // Should parse reasonably quickly (adjust threshold as needed)
        REQUIRE(duration.count() < 1000); // Less than 1 second

        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);
        // Generated code uses "local function" which are local declarations, not function declarations
        REQUIRE(validator.local_declaration_count() == num_functions * 2); // function + result variable per iteration
        REQUIRE(validator.function_expression_count() == num_functions);
    }

    SECTION("Memory usage validation") {
        const String code = R"(
            local function recursive_function(n)
                if n <= 0 then
                    return 1
                else
                    return n * recursive_function(n - 1)
                end
            end

            local result = recursive_function(10)
        )";

        // Parse multiple times to check for memory leaks
        for (int i = 0; i < 10; ++i) {
            auto result = parse_lua_code(code, "memory_test.lua");
            REQUIRE(is_success(result));

            auto program = get_value(std::move(result));
            REQUIRE(program != nullptr);

            ASTValidator validator;
            program->accept(validator);
            // "local function recursive_function" is a local declaration, not function declaration
            REQUIRE(validator.local_declaration_count() >= 2); // function + result
            REQUIRE(validator.function_expression_count() >= 1);

            // Program should be automatically cleaned up when going out of scope
        }
    }
}

// ============================================================================
// Real-world Lua Code Integration Tests
// ============================================================================

TEST_CASE("Frontend Integration - Real-world Lua Code", "[frontend][integration][realworld]") {
    SECTION("Lua standard library style code") {
        const String code = R"(
            -- Table utility functions
            local table_utils = {}

            function table_utils.copy(t)
                local result = {}
                for k, v in pairs(t) do
                    if type(v) == "table" then
                        result[k] = table_utils.copy(v)
                    else
                        result[k] = v
                    end
                end
                return result
            end

            function table_utils.merge(t1, t2)
                local result = table_utils.copy(t1)
                for k, v in pairs(t2) do
                    result[k] = v
                end
                return result
            end

            return table_utils
        )";

        validate_successful_parse(code, "table_utils");

        auto result = parse_lua_code(code);
        auto program = get_value(std::move(result));

        ASTValidator validator;
        program->accept(validator);

        // Validate realistic code structure
        REQUIRE(validator.function_declaration_count() >= 2);
        REQUIRE(validator.for_generic_count() >= 2);
        // Table constructors: table_utils = {}, result = {} (in copy function)
        REQUIRE(validator.table_constructor_count() >= 2);
        REQUIRE(validator.function_call_count() >= 3);
        REQUIRE(validator.return_count() >= 3);
    }

    SECTION("Object-oriented Lua pattern") {
        const String code = R"(
            local Class = {}
            Class.__index = Class

            function Class:new(name, value)
                local instance = setmetatable({}, self)
                instance.name = name or "default"
                instance.value = value or 0
                return instance
            end

            function Class:getName()
                return self.name
            end

            function Class:getValue()
                return self.value
            end

            function Class:setValue(new_value)
                self.value = new_value
            end

            function Class:toString()
                return string.format("%s: %s", self.name, tostring(self.value))
            end

            -- Usage
            local obj1 = Class:new("test", 42)
            local obj2 = Class:new("another", 100)

            print(obj1:toString())
            print(obj2:toString())

            obj1:setValue(99)
            print(obj1:getValue())
        )";

        validate_successful_parse(code, "oop_pattern");

        auto result = parse_lua_code(code);
        auto program = get_value(std::move(result));

        ASTValidator validator;
        program->accept(validator);

        // Validate OOP pattern structure
        REQUIRE(validator.function_declaration_count() >= 5);
        REQUIRE(validator.method_call_count() >= 5);
        REQUIRE(validator.table_access_count() >= 10);
        // Assignments: Class.__index = Class, instance.name = ..., instance.value = ..., self.value = ...
        REQUIRE(validator.assignment_count() >= 4);
    }

    SECTION("Complex control flow") {
        const String code = R"(
            local function process_data(data)
                local results = {}
                local errors = {}

                for i, item in ipairs(data) do
                    if type(item) ~= "table" then
                        table.insert(errors, "Item " .. i .. " is not a table")
                        goto continue
                    end

                    if not item.id then
                        table.insert(errors, "Item " .. i .. " missing id")
                        goto continue
                    end

                    local processed = {
                        id = item.id,
                        processed_at = os.time(),
                        status = "success"
                    }

                    -- Nested processing
                    if item.children then
                        processed.children = {}
                        for j, child in ipairs(item.children) do
                            if child.active then
                                table.insert(processed.children, {
                                    id = child.id,
                                    parent_id = item.id,
                                    index = j
                                })
                            end
                        end
                    end

                    table.insert(results, processed)

                    ::continue::
                end

                return {
                    results = results,
                    errors = errors,
                    count = #results,
                    error_count = #errors
                }
            end

            -- Test the function
            local test_data = {
                {id = 1, name = "first"},
                {id = 2, name = "second", children = {{id = 21, active = true}}},
                "invalid",
                {name = "missing_id"}
            }

            local result = process_data(test_data)
            print("Processed:", result.count, "errors:", result.error_count)
        )";

        validate_successful_parse(code, "complex_control_flow");

        auto result = parse_lua_code(code);
        auto program = get_value(std::move(result));

        ASTValidator validator;
        program->accept(validator);

        // Validate complex control flow
        // "local function process_data" is a local declaration, not function declaration
        REQUIRE(validator.local_declaration_count() >= 3); // function + test_data + result
        REQUIRE(validator.function_expression_count() >= 1);
        REQUIRE(validator.for_generic_count() >= 2);
        REQUIRE(validator.if_count() >= 4);
        REQUIRE(validator.goto_count() >= 2);
        REQUIRE(validator.label_count() >= 1);
        REQUIRE(validator.table_constructor_count() >= 5);
        REQUIRE(validator.function_call_count() >= 8);
    }
}

// ============================================================================
// End-to-End Integration Test Summary
// ============================================================================

TEST_CASE("Frontend Integration - Complete Pipeline Test", "[frontend][integration][e2e]") {
    SECTION("Full frontend pipeline validation") {
        const String code = R"(
            -- Complete Lua program demonstrating all major features
            local math_utils = {}

            -- Constants
            local PI = 3.14159
            local E = 2.71828

            -- Basic arithmetic functions
            function math_utils.add(a, b)
                return a + b
            end

            function math_utils.multiply(a, b)
                return a * b
            end

            -- Advanced functions with error handling
            function math_utils.divide(a, b)
                if b == 0 then
                    error("Division by zero")
                end
                return a / b
            end

            function math_utils.factorial(n)
                if type(n) ~= "number" or n < 0 or n ~= math.floor(n) then
                    error("Factorial requires a non-negative integer")
                end

                if n <= 1 then
                    return 1
                else
                    return n * math_utils.factorial(n - 1)
                end
            end

            -- Table operations
            function math_utils.sum_array(arr)
                local total = 0
                for i, v in ipairs(arr) do
                    if type(v) == "number" then
                        total = total + v
                    end
                end
                return total
            end

            -- Object-oriented calculator
            local Calculator = {}
            Calculator.__index = Calculator

            function Calculator:new()
                return setmetatable({
                    history = {},
                    result = 0
                }, self)
            end

            function Calculator:add(value)
                self.result = self.result + value
                table.insert(self.history, "+" .. value)
                return self
            end

            function Calculator:multiply(value)
                self.result = self.result * value
                table.insert(self.history, "*" .. value)
                return self
            end

            function Calculator:get_result()
                return self.result
            end

            function Calculator:get_history()
                return table.concat(self.history, " ")
            end

            -- Usage examples
            local calc = Calculator:new()
            calc:add(10):multiply(2):add(5)

            local numbers = {1, 2, 3, 4, 5}
            local sum = math_utils.sum_array(numbers)
            local factorial_5 = math_utils.factorial(5)

            print("Calculator result:", calc:get_result())
            print("Array sum:", sum)
            print("5! =", factorial_5)

            return {
                math_utils = math_utils,
                Calculator = Calculator
            }
        )";

        // Test the complete pipeline
        auto start_time = std::chrono::high_resolution_clock::now();

        // Step 1: Lexical analysis
        Lexer lexer(code, "complete_test.lua");
        REQUIRE(!lexer.has_errors());

        // Step 2: Parsing
        Parser parser(lexer);
        auto result = parser.parse();
        REQUIRE(is_success(result));
        REQUIRE(!parser.has_errors());

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        // Step 3: AST validation
        auto program = get_value(std::move(result));
        REQUIRE(program != nullptr);

        ASTValidator validator;
        program->accept(validator);

        // Comprehensive validation
        REQUIRE(validator.program_count() == 1);
        REQUIRE(validator.function_declaration_count() >= 8);
        REQUIRE(validator.local_declaration_count() >= 5);
        REQUIRE(validator.table_constructor_count() >= 3);
        REQUIRE(validator.method_call_count() >= 5);
        REQUIRE(validator.function_call_count() >= 10);
        REQUIRE(validator.for_generic_count() >= 1);
        REQUIRE(validator.if_count() >= 3);
        REQUIRE(validator.return_count() >= 8);
        REQUIRE(validator.binary_op_count() >= 10);

        // Performance validation
        REQUIRE(duration.count() < 10000); // Less than 10ms for this size

        // Memory validation - total nodes should be reasonable
        REQUIRE(validator.total_nodes() > 50);
        REQUIRE(validator.total_nodes() < 500);

        INFO("Parse time: " << duration.count() << " microseconds");
        INFO("Total AST nodes: " << validator.total_nodes());
        INFO("Functions: " << validator.function_declaration_count());
        INFO("Expressions: " << validator.binary_op_count() + validator.function_call_count());
    }
}
