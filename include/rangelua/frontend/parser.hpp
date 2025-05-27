#pragma once

/**
 * @file parser.hpp
 * @brief Recursive descent parser for Lua syntax
 * @version 0.1.0
 */

#include <vector>

#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/types.hpp"
#include "ast.hpp"
#include "lexer.hpp"

namespace rangelua::frontend {

    /**
     * @brief Parser configuration options
     */
    struct ParserConfig {
        Size max_parse_depth = 1000;
        Size max_expression_depth = 200;
        bool allow_goto = true;
        bool strict_mode = false;
        bool lua_5_1_compat = false;
        bool lua_5_2_compat = false;
        bool lua_5_3_compat = true;
        bool lua_5_4_compat = true;
    };

    /**
     * @brief Recursive descent parser for Lua source code
     *
     * This parser implements a clean separation of concerns where it ONLY handles
     * syntax analysis and AST construction. It does NOT perform any register
     * allocation, bytecode generation, or other code generation tasks.
     *
     * Key design principles:
     * - Pure syntax analysis with no code generation concerns
     * - Proper error recovery and reporting
     * - Support for all Lua 5.5 syntax features
     * - Extensible for future language features
     */
    class Parser {
    public:
        using AST = ProgramPtr;  // For concept compliance

        explicit Parser(Lexer& lexer, ParserConfig config = {});
        explicit Parser(StringView source, String filename = "<input>", ParserConfig config = {});

        ~Parser();

        // Non-copyable, movable
        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;
        Parser(Parser&&) noexcept = default;
        Parser& operator=(Parser&&) noexcept = default;

        /**
         * @brief Parse the input and return AST
         * @return Program AST or error
         */
        Result<ProgramPtr> parse();

        /**
         * @brief Parse a single expression
         * @return Expression AST or error
         */
        Result<ExpressionPtr> parse_expression();

        /**
         * @brief Parse a single statement
         * @return Statement AST or error
         */
        Result<StatementPtr> parse_statement();

        /**
         * @brief Check if parser has encountered errors
         * @return true if errors occurred
         */
        [[nodiscard]] bool has_errors() const noexcept;

        /**
         * @brief Get list of parser errors
         * @return Vector of syntax errors
         */
        [[nodiscard]] const std::vector<SyntaxError>& errors() const noexcept;

        /**
         * @brief Get current parser configuration
         * @return Parser configuration
         */
        [[nodiscard]] const ParserConfig& config() const noexcept;

    private:
        class Impl;
        UniquePtr<Impl> impl_;
    };

    /**
     * @brief Operator precedence levels for expression parsing
     */
    enum class Precedence : std::uint8_t {
        None = 0,
        Or,          // or
        And,         // and
        Equality,    // == ~=
        Comparison,  // < > <= >=
        BitwiseOr,   // |
        BitwiseXor,  // ~
        BitwiseAnd,  // &
        Shift,       // << >>
        Concat,      // ..
        Term,        // + -
        Factor,      // * / // %
        Unary,       // not # - ~
        Power,       // ^
        Call,        // . [] ()
        Primary
    };

    /**
     * @brief Parse rule for Pratt parser implementation
     */
    struct ParseRule {
        using PrefixFn = std::function<ExpressionPtr()>;
        using InfixFn = std::function<ExpressionPtr(ExpressionPtr)>;

        PrefixFn prefix = nullptr;
        InfixFn infix = nullptr;
        Precedence precedence = Precedence::None;
    };

    /**
     * @brief Utility functions for parsing
     */
    namespace parser_utils {

        /**
         * @brief Get operator precedence for token
         * @param type Token type
         * @return Precedence level
         */
        Precedence get_precedence(TokenType type) noexcept;

        /**
         * @brief Check if token is a binary operator
         * @param type Token type
         * @return true if binary operator
         */
        bool is_binary_operator(TokenType type) noexcept;

        /**
         * @brief Check if token is a unary operator
         * @param type Token type
         * @return true if unary operator
         */
        bool is_unary_operator(TokenType type) noexcept;

        /**
         * @brief Convert token to binary operator
         * @param type Token type
         * @return Binary operator or nullopt
         */
        Optional<BinaryOpExpression::Operator> token_to_binary_op(TokenType type) noexcept;

        /**
         * @brief Convert token to unary operator
         * @param type Token type
         * @return Unary operator or nullopt
         */
        Optional<UnaryOpExpression::Operator> token_to_unary_op(TokenType type) noexcept;

        /**
         * @brief Check if token can start an expression
         * @param type Token type
         * @return true if can start expression
         */
        bool can_start_expression(TokenType type) noexcept;

        /**
         * @brief Check if token can start a statement
         * @param type Token type
         * @return true if can start statement
         */
        bool can_start_statement(TokenType type) noexcept;

        /**
         * @brief Check if token is a statement terminator
         * @param type Token type
         * @return true if statement terminator
         */
        bool is_statement_terminator(TokenType type) noexcept;

    }  // namespace parser_utils

    /**
     * @brief AST builder helper class with complete Lua 5.5 support
     */
    class ASTBuilder {
    public:
        // Expression builders
        static ExpressionPtr make_literal(LiteralExpression::Value value,
                                          SourceLocation location = {});
        static ExpressionPtr make_identifier(String name, SourceLocation location = {});
        static ExpressionPtr make_binary_op(BinaryOpExpression::Operator op,
                                            ExpressionPtr left,
                                            ExpressionPtr right,
                                            SourceLocation location = {});
        static ExpressionPtr make_unary_op(UnaryOpExpression::Operator op,
                                           ExpressionPtr operand,
                                           SourceLocation location = {});
        static ExpressionPtr make_function_call(ExpressionPtr function,
                                                ExpressionList arguments,
                                                SourceLocation location = {});
        static ExpressionPtr make_method_call(ExpressionPtr object,
                                              String method_name,
                                              ExpressionList arguments,
                                              SourceLocation location = {});
        static ExpressionPtr make_table_access(ExpressionPtr table,
                                               ExpressionPtr key,
                                               bool is_dot_notation = false,
                                               SourceLocation location = {});
        static ExpressionPtr make_table_constructor(TableConstructorExpression::FieldList fields,
                                                    SourceLocation location = {});
        static ExpressionPtr make_function_expression(FunctionExpression::ParameterList parameters,
                                                      StatementPtr body,
                                                      SourceLocation location = {});
        static ExpressionPtr make_vararg(SourceLocation location = {});
        static ExpressionPtr make_parenthesized(ExpressionPtr expression,
                                                SourceLocation location = {});

        // Statement builders
        static StatementPtr make_block(StatementList statements, SourceLocation location = {});
        static StatementPtr make_assignment(ExpressionList targets,
                                            ExpressionList values,
                                            SourceLocation location = {});
        static StatementPtr make_local_declaration(std::vector<String> names,
                                                   ExpressionList values = {},
                                                   SourceLocation location = {});
        static StatementPtr make_function_declaration(ExpressionPtr name,
                                                      FunctionExpression::ParameterList parameters,
                                                      StatementPtr body,
                                                      bool is_local = false,
                                                      SourceLocation location = {});
        static StatementPtr make_if(ExpressionPtr condition,
                                    StatementPtr then_body,
                                    std::vector<IfStatement::ElseIfClause> elseif_clauses = {},
                                    StatementPtr else_body = nullptr,
                                    SourceLocation location = {});
        static StatementPtr
        make_while(ExpressionPtr condition, StatementPtr body, SourceLocation location = {});
        static StatementPtr make_for_numeric(String variable,
                                             ExpressionPtr start,
                                             ExpressionPtr stop,
                                             ExpressionPtr step,
                                             StatementPtr body,
                                             SourceLocation location = {});
        static StatementPtr make_for_generic(std::vector<String> variables,
                                             ExpressionList expressions,
                                             StatementPtr body,
                                             SourceLocation location = {});
        static StatementPtr
        make_repeat(StatementPtr body, ExpressionPtr condition, SourceLocation location = {});
        static StatementPtr make_do(StatementPtr body, SourceLocation location = {});
        static StatementPtr make_return(ExpressionList values = {}, SourceLocation location = {});
        static StatementPtr make_break(SourceLocation location = {});
        static StatementPtr make_goto(String label, SourceLocation location = {});
        static StatementPtr make_label(String name, SourceLocation location = {});
        static StatementPtr make_expression_statement(ExpressionPtr expression,
                                                      SourceLocation location = {});

        // Program builder
        static ProgramPtr make_program(StatementList statements, SourceLocation location = {});
    };

    /**
     * @brief Parser error recovery strategies
     */
    class ErrorRecovery {
    public:
        enum class Strategy : std::uint8_t {
            Panic,        // Skip to next statement
            Synchronize,  // Find synchronization point
            Insert,       // Insert missing token
            Delete        // Delete unexpected token
        };

        static void recover_from_error(Parser& parser, Strategy strategy);
        static bool is_synchronization_point(TokenType type) noexcept;
        static TokenType suggest_missing_token(TokenType expected, TokenType actual) noexcept;
    };

}  // namespace rangelua::frontend

// Concept verification (commented out until concepts are properly implemented)
// static_assert(rangelua::concepts::Parser<rangelua::frontend::Parser>);
