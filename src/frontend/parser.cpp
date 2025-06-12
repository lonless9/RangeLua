/**
 * @file parser.cpp
 * @brief Parser implementation stub
 * @version 0.1.0
 */

#include <rangelua/frontend/parser.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::frontend {

    // Parser implementation
    class Parser::Impl {
    public:
        explicit Impl(Lexer& lexer, ParserConfig config)
            : lexer_(lexer), config_(std::move(config)), current_token_(lexer_.next_token()) {}

        // Constructor that takes ownership of lexer
        explicit Impl(UniquePtr<Lexer> owned_lexer, ParserConfig config)
            : owned_lexer_(std::move(owned_lexer)),
              lexer_(*owned_lexer_),
              config_(std::move(config)),
              current_token_(lexer_.next_token()) {}

        Result<ProgramPtr> parse() {
            PARSER_LOG_INFO("Starting parse of program");

            StatementList statements;

            // Skip initial newlines and comments
            skip_newlines_and_comments();

            while (!is_at_end()) {
                auto stmt_result = parse_statement();
                if (is_success(stmt_result)) {
                    statements.push_back(get_value(std::move(stmt_result)));
                } else {
                    // Error recovery: try to synchronize to next statement
                    add_error("Failed to parse statement", current_location());
                    synchronize();
                }

                skip_newlines_and_comments();
            }

            PARSER_LOG_INFO("Completed parsing {} statements", statements.size());
            return ASTBuilder::make_program(std::move(statements), SourceLocation{lexer_.filename(), 1, 1});
        }

        Result<ExpressionPtr> parse_expression() {
            return parse_expression_with_precedence(Precedence::None);
        }

        Result<StatementPtr> parse_statement() {
            skip_newlines_and_comments();

            if (is_at_end()) {
                return ErrorCode::SYNTAX_ERROR;
            }

            PARSER_LOG_DEBUG("Parsing statement starting with token: {}",
                             token_type_to_string(current_token_.type));

            // Check for statement keywords
            switch (current_token_.type) {
                case TokenType::Local:
                    PARSER_LOG_DEBUG("Parsing local statement");
                    return parse_local_statement();
                case TokenType::Function:
                    PARSER_LOG_DEBUG("Parsing function declaration");
                    return parse_function_declaration(false);
                case TokenType::If:
                    PARSER_LOG_DEBUG("Parsing if statement");
                    return parse_if_statement();
                case TokenType::While:
                    PARSER_LOG_DEBUG("Parsing while statement");
                    return parse_while_statement();
                case TokenType::For:
                    PARSER_LOG_DEBUG("Parsing for statement");
                    return parse_for_statement();
                case TokenType::Repeat:
                    PARSER_LOG_DEBUG("Parsing repeat statement");
                    return parse_repeat_statement();
                case TokenType::Do:
                    PARSER_LOG_DEBUG("Parsing do statement");
                    return parse_do_statement();
                case TokenType::Return:
                    PARSER_LOG_DEBUG("Parsing return statement");
                    return parse_return_statement();
                case TokenType::Break:
                    PARSER_LOG_DEBUG("Parsing break statement");
                    return parse_break_statement();
                case TokenType::Goto:
                    PARSER_LOG_DEBUG("Parsing goto statement");
                    return parse_goto_statement();
                case TokenType::DoubleColon:
                    PARSER_LOG_DEBUG("Parsing label statement");
                    return parse_label_statement();
                default:
                    PARSER_LOG_DEBUG("Parsing assignment or expression statement");
                    // Try to parse as assignment or expression statement
                    return parse_assignment_or_expression_statement();
            }
        }

        bool has_errors() const noexcept { return !errors_.empty(); }

        const std::vector<SyntaxError>& errors() const noexcept { return errors_; }

        const ParserConfig& config() const noexcept { return config_; }

    private:
        UniquePtr<Lexer> owned_lexer_;  // Optional owned lexer
        Lexer& lexer_;
        ParserConfig config_;
        std::vector<SyntaxError> errors_;
        Token current_token_;
        [[maybe_unused]] Size parse_dbepth_ = 0;
        Size expression_depth_ = 0;

        // Token management
        void advance() {
            if (!is_at_end()) {
                current_token_ = lexer_.next_token();
            }
        }

        bool is_at_end() const noexcept { return current_token_.type == TokenType::EndOfFile; }

        bool check(TokenType type) const noexcept { return current_token_.type == type; }

        bool match(TokenType type) {
            if (check(type)) {
                advance();
                return true;
            }
            return false;
        }

        bool match_any(std::initializer_list<TokenType> types) {
            for (auto type : types) {
                if (match(type)) {
                    return true;
                }
            }
            return false;
        }

        SourceLocation current_location() const { return current_token_.location; }

        void skip_newlines_and_comments() {
            while (match_any({TokenType::Newline, TokenType::Comment})) {
                // Continue skipping
            }
        }

        // Error handling
        void add_error(const String& message, const SourceLocation& location) {
            errors_.emplace_back(message, location);
            PARSER_LOG_ERROR(
                "Parse error at {}:{}: {}", location.filename_, location.line_, message);
        }

        void add_enhanced_error(const String& message,
                                const SourceLocation& location,
                                const String& suggestion = "") {
            String enhanced_message = message;
            if (!suggestion.empty()) {
                enhanced_message += ". Suggestion: " + suggestion;
            }
            errors_.emplace_back(enhanced_message, location);
            PARSER_LOG_ERROR(
                "Parse error at {}:{}: {}", location.filename_, location.line_, enhanced_message);
        }

        String get_token_suggestion(TokenType expected) {
            switch (expected) {
                case TokenType::LeftParen:
                    return "add '(' before the parameter list";
                case TokenType::RightParen:
                    return "add ')' to close the parameter list";
                case TokenType::Identifier:
                    return "provide a valid identifier name";
                case TokenType::Then:
                    return "add 'then' after the condition";
                case TokenType::End:
                    return "add 'end' to close the block";
                default:
                    return "";
            }
        }

        void synchronize() {
            advance();

            while (!is_at_end()) {
                if (current_token_.type == TokenType::Semicolon) {
                    advance();
                    return;
                }

                // Synchronize on statement keywords
                if (parser_utils::can_start_statement(current_token_.type)) {
                    return;
                }

                advance();
            }
        }

        bool expect(TokenType type, const String& message) {
            if (check(type)) {
                advance();
                return true;
            }

            String suggestion = get_token_suggestion(type);
            add_enhanced_error(message, current_location(), suggestion);
            return false;
        }

        // Expression parsing with precedence climbing
        Result<ExpressionPtr> parse_expression_with_precedence(Precedence min_precedence) {
            if (++expression_depth_ > config_.max_expression_depth) {
                add_error("Expression depth limit exceeded", current_location());
                --expression_depth_;
                return ErrorCode::SYNTAX_ERROR;
            }

            auto left_result = parse_primary_expression();
            if (!is_success(left_result)) {
                --expression_depth_;
                return left_result;
            }

            auto left = get_value(std::move(left_result));

            while (!is_at_end()) {
                auto precedence = parser_utils::get_precedence(current_token_.type);
                if (precedence <= min_precedence) {
                    break;
                }

                if (parser_utils::is_binary_operator(current_token_.type)) {
                    auto op_result = parser_utils::token_to_binary_op(current_token_.type);
                    if (!op_result) {
                        break;
                    }

                    auto op_token = current_token_;
                    advance();

                    auto right_result = parse_expression_with_precedence(precedence);
                    if (!is_success(right_result)) {
                        --expression_depth_;
                        return right_result;
                    }

                    left = ASTBuilder::make_binary_op(op_result.value(),
                                                      std::move(left),
                                                      get_value(std::move(right_result)),
                                                      op_token.location);
                } else if (current_token_.type == TokenType::LeftParen) {
                    // Function call
                    auto call_result = parse_function_call(std::move(left));
                    if (!is_success(call_result)) {
                        --expression_depth_;
                        return call_result;
                    }
                    left = get_value(std::move(call_result));
                } else if (current_token_.type == TokenType::LeftBracket) {
                    // Table access with brackets
                    advance();  // consume '['
                    auto key_result = parse_expression();
                    if (!is_success(key_result)) {
                        --expression_depth_;
                        return key_result;
                    }

                    if (!expect(TokenType::RightBracket, "Expected ']' after table key")) {
                        --expression_depth_;
                        return ErrorCode::SYNTAX_ERROR;
                    }

                    left = ASTBuilder::make_table_access(std::move(left),
                                                         get_value(std::move(key_result)),
                                                         false,
                                                         current_location());
                } else if (current_token_.type == TokenType::Dot) {
                    // Table access with dot notation
                    advance();  // consume '.'
                    if (!check(TokenType::Identifier)) {
                        add_error("Expected identifier after '.'", current_location());
                        --expression_depth_;
                        return ErrorCode::SYNTAX_ERROR;
                    }

                    auto field_name = current_token_.value;
                    auto location = current_location();
                    advance();

                    auto key = ASTBuilder::make_literal(field_name, location);
                    left = ASTBuilder::make_table_access(
                        std::move(left), std::move(key), true, location);
                } else if (current_token_.type == TokenType::Colon) {
                    // Method call
                    advance();  // consume ':'
                    if (!check(TokenType::Identifier)) {
                        add_error("Expected method name after ':'", current_location());
                        --expression_depth_;
                        return ErrorCode::SYNTAX_ERROR;
                    }

                    auto method_name = current_token_.value;
                    auto location = current_location();
                    advance();

                    if (!check(TokenType::LeftParen)) {
                        add_error("Expected '(' after method name", current_location());
                        --expression_depth_;
                        return ErrorCode::SYNTAX_ERROR;
                    }

                    auto args_result = parse_argument_list();
                    if (!is_success(args_result)) {
                        --expression_depth_;
                        return ErrorCode::SYNTAX_ERROR;
                    }

                    left = ASTBuilder::make_method_call(
                        std::move(left), method_name, get_value(std::move(args_result)), location);
                } else {
                    break;
                }
            }

            --expression_depth_;
            return left;
        }

        Result<ExpressionPtr> parse_primary_expression() {
            auto location = current_location();

            switch (current_token_.type) {
                case TokenType::Number: {
                    auto value =
                        current_token_.number_value
                            ? LiteralExpression::Value{current_token_.number_value.value()}
                            : LiteralExpression::Value{current_token_.integer_value.value_or(0)};
                    advance();
                    return ASTBuilder::make_literal(std::move(value), location);
                }

                case TokenType::String: {
                    auto value = current_token_.value;
                    advance();
                    return ASTBuilder::make_literal(LiteralExpression::Value{value}, location);
                }

                case TokenType::True: {
                    advance();
                    return ASTBuilder::make_literal(LiteralExpression::Value{true}, location);
                }

                case TokenType::False: {
                    advance();
                    return ASTBuilder::make_literal(LiteralExpression::Value{false}, location);
                }

                case TokenType::Nil: {
                    advance();
                    return ASTBuilder::make_literal(LiteralExpression::Value{std::monostate{}},
                                                    location);
                }

                case TokenType::Identifier: {
                    auto name = current_token_.value;
                    advance();
                    return ASTBuilder::make_identifier(name, location);
                }

                case TokenType::Ellipsis: {
                    advance();
                    return ASTBuilder::make_vararg(location);
                }

                case TokenType::LeftParen: {
                    advance();  // consume '('
                    auto expr_result = parse_expression();
                    if (!is_success(expr_result)) {
                        return expr_result;
                    }

                    if (!expect(TokenType::RightParen, "Expected ')' after expression")) {
                        return ErrorCode::SYNTAX_ERROR;
                    }

                    return ASTBuilder::make_parenthesized(get_value(std::move(expr_result)),
                                                          location);
                }

                case TokenType::LeftBrace: {
                    return parse_table_constructor();
                }

                case TokenType::Function: {
                    return parse_function_expression();
                }

                default:
                    // Check for unary operators
                    if (parser_utils::is_unary_operator(current_token_.type)) {
                        auto op_result = parser_utils::token_to_unary_op(current_token_.type);
                        if (op_result) {
                            advance();
                            auto operand_result =
                                parse_expression_with_precedence(Precedence::Unary);
                            if (!is_success(operand_result)) {
                                return operand_result;
                            }

                            return ASTBuilder::make_unary_op(
                                op_result.value(), get_value(std::move(operand_result)), location);
                        }
                    }

                    add_error("Unexpected token in expression", location);
                    return ErrorCode::SYNTAX_ERROR;
            }
        }

        // Statement parsing methods
        Result<StatementPtr> parse_local_statement() {
            advance();  // consume 'local'

            if (check(TokenType::Function)) {
                return parse_local_function();
            }

            // Parse local variable declaration
            std::vector<String> names;

            if (!check(TokenType::Identifier)) {
                add_error("Expected identifier after 'local'", current_location());
                return ErrorCode::SYNTAX_ERROR;
            }

            names.push_back(current_token_.value);
            advance();

            while (match(TokenType::Comma)) {
                if (!check(TokenType::Identifier)) {
                    add_error("Expected identifier after ','", current_location());
                    return ErrorCode::SYNTAX_ERROR;
                }
                names.push_back(current_token_.value);
                advance();
            }

            ExpressionList values;
            if (match(TokenType::Assign)) {
                auto expr_result = parse_expression();
                if (!is_success(expr_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }
                values.push_back(get_value(std::move(expr_result)));

                while (match(TokenType::Comma)) {
                    auto expr_result = parse_expression();
                    if (!is_success(expr_result)) {
                        return ErrorCode::SYNTAX_ERROR;
                    }
                    values.push_back(get_value(std::move(expr_result)));
                }
            }

            return ASTBuilder::make_local_declaration(
                std::move(names), std::move(values), current_location());
        }

        // Parse local function as local declaration with function expression
        Result<StatementPtr> parse_local_function() {
            advance();  // consume 'function'

            if (!check(TokenType::Identifier)) {
                add_error("Expected function name after 'local function'", current_location());
                return ErrorCode::SYNTAX_ERROR;
            }

            String function_name = current_token_.value;
            advance();

            // Parse parameters
            if (!expect(TokenType::LeftParen, "Expected '(' after function name")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            FunctionExpression::ParameterList parameters;
            if (!check(TokenType::RightParen)) {
                if (check(TokenType::Identifier)) {
                    parameters.emplace_back(current_token_.value);
                    advance();

                    while (match(TokenType::Comma)) {
                        if (check(TokenType::Ellipsis)) {
                            parameters.emplace_back("...", true);
                            advance();
                            break;
                        } else if (check(TokenType::Identifier)) {
                            parameters.emplace_back(current_token_.value);
                            advance();
                        } else {
                            add_error("Expected parameter name", current_location());
                            return ErrorCode::SYNTAX_ERROR;
                        }
                    }
                } else if (check(TokenType::Ellipsis)) {
                    parameters.emplace_back("...", true);
                    advance();
                }
            }

            if (!expect(TokenType::RightParen, "Expected ')' after parameters")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse body
            auto body_result = parse_block_until(TokenType::End);
            if (!is_success(body_result)) {
                return body_result;
            }

            if (!expect(TokenType::End, "Expected 'end' after function body")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Create function expression
            auto function_expr = ASTBuilder::make_function_expression(
                std::move(parameters), get_value(std::move(body_result)), current_location());

            // Create local declaration with function expression as value
            std::vector<String> names = {std::move(function_name)};
            ExpressionList values;
            values.push_back(std::move(function_expr));

            return ASTBuilder::make_local_declaration(
                std::move(names), std::move(values), current_location());
        }

        // Parse function name with support for table.method and obj:method syntax
        Result<ExpressionPtr> parse_function_name() {
            if (!check(TokenType::Identifier)) {
                add_enhanced_error("Expected function name",
                                   current_location(),
                                   "provide a valid function identifier");
                return ErrorCode::SYNTAX_ERROR;
            }

            auto name = ASTBuilder::make_identifier(current_token_.value, current_location());
            advance();

            // Handle table.method syntax
            while (match(TokenType::Dot)) {
                if (!check(TokenType::Identifier)) {
                    add_error("Expected identifier after '.'", current_location());
                    return ErrorCode::SYNTAX_ERROR;
                }

                auto field_name = current_token_.value;
                auto location = current_location();
                advance();

                auto key = ASTBuilder::make_literal(field_name, location);
                name =
                    ASTBuilder::make_table_access(std::move(name), std::move(key), true, location);
            }

            // Handle obj:method syntax
            if (match(TokenType::Colon)) {
                if (!check(TokenType::Identifier)) {
                    add_error("Expected method name after ':'", current_location());
                    return ErrorCode::SYNTAX_ERROR;
                }

                auto method_name = current_token_.value;
                auto location = current_location();
                advance();

                auto key = ASTBuilder::make_literal(method_name, location);
                name =
                    ASTBuilder::make_table_access(std::move(name), std::move(key), true, location);
            }

            return name;
        }

        Result<StatementPtr> parse_function_declaration(bool is_local) {
            advance();  // consume 'function'

            // Parse function name (supports table.method and obj:method syntax)
            auto name_result = parse_function_name();
            if (!is_success(name_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            auto name = get_value(std::move(name_result));

            // Parse parameters
            if (!expect(TokenType::LeftParen, "Expected '(' after function name")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            FunctionExpression::ParameterList parameters;
            if (!check(TokenType::RightParen)) {
                if (check(TokenType::Identifier)) {
                    parameters.emplace_back(current_token_.value);
                    advance();

                    while (match(TokenType::Comma)) {
                        if (check(TokenType::Ellipsis)) {
                            parameters.emplace_back("...", true);
                            advance();
                            break;
                        } else if (check(TokenType::Identifier)) {
                            parameters.emplace_back(current_token_.value);
                            advance();
                        } else {
                            add_enhanced_error("Expected parameter name", current_location(),
                                             "provide a valid parameter identifier");
                            return ErrorCode::SYNTAX_ERROR;
                        }
                    }
                } else if (check(TokenType::Ellipsis)) {
                    parameters.emplace_back("...", true);
                    advance();
                }
            }

            if (!expect(TokenType::RightParen, "Expected ')' after parameters")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse body
            auto body_result = parse_block_until(TokenType::End);
            if (!is_success(body_result)) {
                return body_result;
            }

            if (!expect(TokenType::End, "Expected 'end' after function body")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_function_declaration(std::move(name),
                                                         std::move(parameters),
                                                         get_value(std::move(body_result)),
                                                         is_local,
                                                         current_location());
        }

        Result<StatementPtr> parse_block_until(TokenType end_token) {
            StatementList statements;

            skip_newlines_and_comments();
            while (!is_at_end() && !check(end_token)) {
                auto stmt_result = parse_statement();
                if (is_success(stmt_result)) {
                    statements.push_back(get_value(std::move(stmt_result)));
                } else {
                    synchronize();
                }
                skip_newlines_and_comments();
            }

            return ASTBuilder::make_block(std::move(statements), current_location());
        }

        // Overloaded version for multiple end tokens
        template <typename... TokenTypes>
        Result<StatementPtr> parse_block_until(TokenType first_token, TokenTypes... other_tokens) {
            StatementList statements;

            skip_newlines_and_comments();
            while (!is_at_end() && !check(first_token) && !(check(other_tokens) || ...)) {
                auto stmt_result = parse_statement();
                if (is_success(stmt_result)) {
                    statements.push_back(get_value(std::move(stmt_result)));
                } else {
                    synchronize();
                }
                skip_newlines_and_comments();
            }

            return ASTBuilder::make_block(std::move(statements), current_location());
        }

        Result<StatementPtr> parse_assignment_or_expression_statement() {
            // Try to parse as assignment first
            auto checkpoint_token = current_token_;

            ExpressionList targets;
            auto first_expr_result = parse_expression();
            if (!is_success(first_expr_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            targets.push_back(get_value(std::move(first_expr_result)));

            // Check if this is an assignment
            while (match(TokenType::Comma)) {
                auto expr_result = parse_expression();
                if (!is_success(expr_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }
                targets.push_back(get_value(std::move(expr_result)));
            }

            if (match(TokenType::Assign)) {
                // This is an assignment
                ExpressionList values;
                auto expr_result = parse_expression();
                if (!is_success(expr_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }
                values.push_back(get_value(std::move(expr_result)));

                while (match(TokenType::Comma)) {
                    auto expr_result = parse_expression();
                    if (!is_success(expr_result)) {
                        return ErrorCode::SYNTAX_ERROR;
                    }
                    values.push_back(get_value(std::move(expr_result)));
                }

                return ASTBuilder::make_assignment(
                    std::move(targets), std::move(values), current_location());
            } else {
                // This is an expression statement (only if single expression)
                if (targets.size() == 1) {
                    return ASTBuilder::make_expression_statement(std::move(targets[0]),
                                                                 current_location());
                } else {
                    add_error("Invalid expression statement", current_location());
                    return ErrorCode::SYNTAX_ERROR;
                }
            }
        }

        // Placeholder implementations for other statement types
        Result<StatementPtr> parse_if_statement() {
            advance();  // consume 'if'

            // Parse condition
            auto condition_result = parse_expression();
            if (!is_success(condition_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            if (!expect(TokenType::Then, "Expected 'then' after if condition")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse then body
            auto then_body_result =
                parse_block_until(TokenType::Elseif, TokenType::Else, TokenType::End);
            if (!is_success(then_body_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse elseif clauses
            std::vector<IfStatement::ElseIfClause> elseif_clauses;
            while (match(TokenType::Elseif)) {
                auto elseif_condition_result = parse_expression();
                if (!is_success(elseif_condition_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                if (!expect(TokenType::Then, "Expected 'then' after elseif condition")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                auto elseif_body_result =
                    parse_block_until(TokenType::Elseif, TokenType::Else, TokenType::End);
                if (!is_success(elseif_body_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                IfStatement::ElseIfClause clause;
                clause.condition = get_value(std::move(elseif_condition_result));
                clause.body = get_value(std::move(elseif_body_result));
                elseif_clauses.push_back(std::move(clause));
            }

            // Parse optional else clause
            StatementPtr else_body = nullptr;
            if (match(TokenType::Else)) {
                auto else_body_result = parse_block_until(TokenType::End);
                if (!is_success(else_body_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }
                else_body = get_value(std::move(else_body_result));
            }

            if (!expect(TokenType::End, "Expected 'end' after if statement")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_if(get_value(std::move(condition_result)),
                                       get_value(std::move(then_body_result)),
                                       std::move(elseif_clauses),
                                       std::move(else_body),
                                       current_location());
        }

        Result<StatementPtr> parse_while_statement() {
            advance();  // consume 'while'

            // Parse condition
            auto condition_result = parse_expression();
            if (!is_success(condition_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            if (!expect(TokenType::Do, "Expected 'do' after while condition")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse body
            auto body_result = parse_block_until(TokenType::End);
            if (!is_success(body_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            if (!expect(TokenType::End, "Expected 'end' after while body")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_while(get_value(std::move(condition_result)),
                                          get_value(std::move(body_result)),
                                          current_location());
        }

        Result<StatementPtr> parse_for_statement() {
            advance();  // consume 'for'

            if (!check(TokenType::Identifier)) {
                add_error("Expected variable name after 'for'", current_location());
                return ErrorCode::SYNTAX_ERROR;
            }

            auto first_var = current_token_.value;
            advance();

            if (match(TokenType::Assign)) {
                // Numeric for loop: for var = start, stop [, step] do ... end
                auto start_result = parse_expression();
                if (!is_success(start_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                if (!expect(TokenType::Comma, "Expected ',' after for loop start value")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                auto stop_result = parse_expression();
                if (!is_success(stop_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                ExpressionPtr step = nullptr;
                if (match(TokenType::Comma)) {
                    auto step_result = parse_expression();
                    if (!is_success(step_result)) {
                        return ErrorCode::SYNTAX_ERROR;
                    }
                    step = get_value(std::move(step_result));
                }

                if (!expect(TokenType::Do, "Expected 'do' after for loop range")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                auto body_result = parse_block_until(TokenType::End);
                if (!is_success(body_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                if (!expect(TokenType::End, "Expected 'end' after for loop body")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                return ASTBuilder::make_for_numeric(first_var,
                                                    get_value(std::move(start_result)),
                                                    get_value(std::move(stop_result)),
                                                    std::move(step),
                                                    get_value(std::move(body_result)),
                                                    current_location());
            } else {
                // Generic for loop: for vars in explist do ... end
                std::vector<String> variables;
                variables.push_back(first_var);

                while (match(TokenType::Comma)) {
                    if (!check(TokenType::Identifier)) {
                        add_error("Expected variable name after ','", current_location());
                        return ErrorCode::SYNTAX_ERROR;
                    }
                    variables.push_back(current_token_.value);
                    advance();
                }

                if (!expect(TokenType::In, "Expected 'in' after for loop variables")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                ExpressionList expressions;
                auto expr_result = parse_expression();
                if (!is_success(expr_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }
                expressions.push_back(get_value(std::move(expr_result)));

                while (match(TokenType::Comma)) {
                    auto expr_result = parse_expression();
                    if (!is_success(expr_result)) {
                        return ErrorCode::SYNTAX_ERROR;
                    }
                    expressions.push_back(get_value(std::move(expr_result)));
                }

                if (!expect(TokenType::Do, "Expected 'do' after for loop expressions")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                auto body_result = parse_block_until(TokenType::End);
                if (!is_success(body_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                if (!expect(TokenType::End, "Expected 'end' after for loop body")) {
                    return ErrorCode::SYNTAX_ERROR;
                }

                return ASTBuilder::make_for_generic(std::move(variables),
                                                    std::move(expressions),
                                                    get_value(std::move(body_result)),
                                                    current_location());
            }
        }

        Result<StatementPtr> parse_repeat_statement() {
            advance();  // consume 'repeat'

            // Parse body
            auto body_result = parse_block_until(TokenType::Until);
            if (!is_success(body_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            if (!expect(TokenType::Until, "Expected 'until' after repeat body")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse condition
            auto condition_result = parse_expression();
            if (!is_success(condition_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_repeat(get_value(std::move(body_result)),
                                           get_value(std::move(condition_result)),
                                           current_location());
        }

        Result<StatementPtr> parse_do_statement() {
            advance();  // consume 'do'

            // Parse body
            auto body_result = parse_block_until(TokenType::End);
            if (!is_success(body_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            if (!expect(TokenType::End, "Expected 'end' after do body")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_do(get_value(std::move(body_result)), current_location());
        }

        Result<StatementPtr> parse_return_statement() {
            advance();  // consume 'return'

            ExpressionList values;
            if (!parser_utils::is_statement_terminator(current_token_.type) && !is_at_end()) {
                auto expr_result = parse_expression();
                if (is_success(expr_result)) {
                    values.push_back(get_value(std::move(expr_result)));

                    while (match(TokenType::Comma)) {
                        auto expr_result = parse_expression();
                        if (is_success(expr_result)) {
                            values.push_back(get_value(std::move(expr_result)));
                        } else {
                            break;
                        }
                    }
                }
            }

            return ASTBuilder::make_return(std::move(values), current_location());
        }

        Result<StatementPtr> parse_break_statement() {
            advance();  // consume 'break'
            return ASTBuilder::make_break(current_location());
        }

        Result<StatementPtr> parse_goto_statement() {
            advance();  // consume 'goto'

            if (!check(TokenType::Identifier)) {
                add_error("Expected label name after 'goto'", current_location());
                return ErrorCode::SYNTAX_ERROR;
            }

            auto label = current_token_.value;
            advance();

            return ASTBuilder::make_goto(label, current_location());
        }

        Result<StatementPtr> parse_label_statement() {
            advance();  // consume '::'

            if (!check(TokenType::Identifier)) {
                add_error("Expected label name after '::'", current_location());
                return ErrorCode::SYNTAX_ERROR;
            }

            auto label = current_token_.value;
            advance();

            if (!expect(TokenType::DoubleColon, "Expected '::' after label name")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_label(label, current_location());
        }

        // Expression parsing helper methods
        Result<ExpressionPtr> parse_function_call(ExpressionPtr function) {
            auto args_result = parse_argument_list();
            if (!is_success(args_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_function_call(
                std::move(function), get_value(std::move(args_result)), current_location());
        }

        Result<ExpressionList> parse_argument_list() {
            if (!expect(TokenType::LeftParen, "Expected '(' for argument list")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            ExpressionList arguments;
            if (!check(TokenType::RightParen)) {
                auto expr_result = parse_expression();
                if (!is_success(expr_result)) {
                    return ErrorCode::SYNTAX_ERROR;
                }
                arguments.push_back(get_value(std::move(expr_result)));

                while (match(TokenType::Comma)) {
                    auto expr_result = parse_expression();
                    if (!is_success(expr_result)) {
                        return ErrorCode::SYNTAX_ERROR;
                    }
                    arguments.push_back(get_value(std::move(expr_result)));
                }
            }

            if (!expect(TokenType::RightParen, "Expected ')' after arguments")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return arguments;
        }

        Result<ExpressionPtr> parse_table_constructor() {
            advance();  // consume '{'

            TableConstructorExpression::FieldList fields;

            if (!check(TokenType::RightBrace)) {
                do {
                    // Parse field
                    if (check(TokenType::LeftBracket)) {
                        // Array field: [key] = value
                        advance();  // consume '['
                        auto key_result = parse_expression();
                        if (!is_success(key_result)) {
                            return ErrorCode::SYNTAX_ERROR;
                        }

                        if (!expect(TokenType::RightBracket, "Expected ']' after table key")) {
                            return ErrorCode::SYNTAX_ERROR;
                        }

                        if (!expect(TokenType::Assign, "Expected '=' after table key")) {
                            return ErrorCode::SYNTAX_ERROR;
                        }

                        auto value_result = parse_expression();
                        if (!is_success(value_result)) {
                            return ErrorCode::SYNTAX_ERROR;
                        }

                        fields.emplace_back(TableConstructorExpression::Field::Type::Array,
                                            get_value(std::move(key_result)),
                                            get_value(std::move(value_result)));
                    } else if (check(TokenType::Identifier)) {
                        // Could be record field (name = value) or list field (value)
                        auto checkpoint_token = current_token_;
                        advance();

                        if (match(TokenType::Assign)) {
                            // Record field: name = value
                            auto key = ASTBuilder::make_literal(
                                LiteralExpression::Value{checkpoint_token.value},
                                checkpoint_token.location);
                            auto value_result = parse_expression();
                            if (!is_success(value_result)) {
                                return ErrorCode::SYNTAX_ERROR;
                            }

                            fields.emplace_back(TableConstructorExpression::Field::Type::Record,
                                                std::move(key),
                                                get_value(std::move(value_result)));
                        } else {
                            // List field: treat identifier as expression
                            // Backtrack and parse as expression
                            current_token_ = checkpoint_token;
                            auto value_result = parse_expression();
                            if (!is_success(value_result)) {
                                return ErrorCode::SYNTAX_ERROR;
                            }

                            fields.emplace_back(TableConstructorExpression::Field::Type::List,
                                                nullptr,
                                                get_value(std::move(value_result)));
                        }
                    } else {
                        // List field: value
                        auto value_result = parse_expression();
                        if (!is_success(value_result)) {
                            return ErrorCode::SYNTAX_ERROR;
                        }

                        fields.emplace_back(TableConstructorExpression::Field::Type::List,
                                            nullptr,
                                            get_value(std::move(value_result)));
                    }

                    // Optional comma or semicolon separator
                    if (!match(TokenType::Comma) && !match(TokenType::Semicolon)) {
                        break;
                    }
                } while (!check(TokenType::RightBrace) && !is_at_end());
            }

            if (!expect(TokenType::RightBrace, "Expected '}' after table constructor")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_table_constructor(std::move(fields), current_location());
        }

        Result<ExpressionPtr> parse_function_expression() {
            advance();  // consume 'function'

            if (!expect(TokenType::LeftParen, "Expected '(' after 'function'")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse parameters
            FunctionExpression::ParameterList parameters;
            if (!check(TokenType::RightParen)) {
                if (check(TokenType::Identifier)) {
                    parameters.emplace_back(current_token_.value);
                    advance();

                    while (match(TokenType::Comma)) {
                        if (check(TokenType::Ellipsis)) {
                            parameters.emplace_back("...", true);
                            advance();
                            break;
                        } else if (check(TokenType::Identifier)) {
                            parameters.emplace_back(current_token_.value);
                            advance();
                        } else {
                            add_error("Expected parameter name", current_location());
                            return ErrorCode::SYNTAX_ERROR;
                        }
                    }
                } else if (check(TokenType::Ellipsis)) {
                    parameters.emplace_back("...", true);
                    advance();
                }
            }

            if (!expect(TokenType::RightParen, "Expected ')' after parameters")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            // Parse body
            auto body_result = parse_block_until(TokenType::End);
            if (!is_success(body_result)) {
                return ErrorCode::SYNTAX_ERROR;
            }

            if (!expect(TokenType::End, "Expected 'end' after function body")) {
                return ErrorCode::SYNTAX_ERROR;
            }

            return ASTBuilder::make_function_expression(
                std::move(parameters), get_value(std::move(body_result)), current_location());
        }
    };

    Parser::Parser(Lexer& lexer, ParserConfig config)
        : impl_(std::make_unique<Impl>(lexer, std::move(config))) {}

    Parser::Parser(StringView source, String filename, ParserConfig config) {
        auto lexer = std::make_unique<Lexer>(source, filename);
        impl_ = std::make_unique<Impl>(std::move(lexer), config);
    }

    Parser::~Parser() = default;

    Result<ProgramPtr> Parser::parse() {
        return impl_->parse();
    }

    Result<ExpressionPtr> Parser::parse_expression() {
        return impl_->parse_expression();
    }

    Result<StatementPtr> Parser::parse_statement() {
        return impl_->parse_statement();
    }

    bool Parser::has_errors() const noexcept {
        return impl_->has_errors();
    }

    const std::vector<SyntaxError>& Parser::errors() const noexcept {
        return impl_->errors();
    }

    const ParserConfig& Parser::config() const noexcept {
        return impl_->config();
    }

    // AST implementations
    void LiteralExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String LiteralExpression::to_string() const {
        return "LiteralExpression";
    }

    void IdentifierExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String IdentifierExpression::to_string() const {
        return "IdentifierExpression(" + name_ + ")";
    }

    void BinaryOpExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String BinaryOpExpression::to_string() const {
        return "BinaryOpExpression";
    }

    void UnaryOpExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String UnaryOpExpression::to_string() const {
        return "UnaryOpExpression";
    }

    void FunctionCallExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String FunctionCallExpression::to_string() const {
        return "FunctionCallExpression";
    }

    void BlockStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String BlockStatement::to_string() const {
        return "BlockStatement";
    }

    void AssignmentStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String AssignmentStatement::to_string() const {
        return "AssignmentStatement";
    }

    void IfStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String IfStatement::to_string() const {
        return "IfStatement";
    }

    void Program::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String Program::to_string() const {
        return "Program";
    }

    // New AST node implementations
    void MethodCallExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String MethodCallExpression::to_string() const {
        return "MethodCallExpression(" + method_name_ + ")";
    }

    void TableAccessExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String TableAccessExpression::to_string() const {
        return is_dot_notation_ ? "TableAccessExpression(dot)" : "TableAccessExpression(bracket)";
    }

    void TableConstructorExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String TableConstructorExpression::to_string() const {
        return "TableConstructorExpression";
    }

    void FunctionExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String FunctionExpression::to_string() const {
        return "FunctionExpression";
    }

    void VarargExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String VarargExpression::to_string() const {
        return "VarargExpression";
    }

    void ParenthesizedExpression::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String ParenthesizedExpression::to_string() const {
        return "ParenthesizedExpression";
    }

    void LocalDeclarationStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String LocalDeclarationStatement::to_string() const {
        return "LocalDeclarationStatement";
    }

    void FunctionDeclarationStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String FunctionDeclarationStatement::to_string() const {
        return is_local_ ? "LocalFunctionDeclarationStatement" : "FunctionDeclarationStatement";
    }

    void WhileStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String WhileStatement::to_string() const {
        return "WhileStatement";
    }

    void ForNumericStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String ForNumericStatement::to_string() const {
        return "ForNumericStatement";
    }

    void ForGenericStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String ForGenericStatement::to_string() const {
        return "ForGenericStatement";
    }

    void RepeatStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String RepeatStatement::to_string() const {
        return "RepeatStatement";
    }

    void DoStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String DoStatement::to_string() const {
        return "DoStatement";
    }

    void ReturnStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String ReturnStatement::to_string() const {
        return "ReturnStatement";
    }

    void BreakStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String BreakStatement::to_string() const {
        return "BreakStatement";
    }

    void GotoStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String GotoStatement::to_string() const {
        return "GotoStatement(" + label_ + ")";
    }

    void LabelStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String LabelStatement::to_string() const {
        return "LabelStatement(" + name_ + ")";
    }

    void ExpressionStatement::accept(ASTVisitor& visitor) const {
        visitor.visit(*this);
    }

    String ExpressionStatement::to_string() const {
        return "ExpressionStatement";
    }

    // Utility functions
    namespace parser_utils {

        Precedence get_precedence(TokenType type) noexcept {
            switch (type) {
                case TokenType::Or:
                    return Precedence::Or;
                case TokenType::And:
                    return Precedence::And;
                case TokenType::Equal:
                case TokenType::NotEqual:
                    return Precedence::Equality;
                case TokenType::Less:
                case TokenType::LessEqual:
                case TokenType::Greater:
                case TokenType::GreaterEqual:
                    return Precedence::Comparison;
                case TokenType::BitwiseOr:
                    return Precedence::BitwiseOr;
                case TokenType::BitwiseXor:
                    return Precedence::BitwiseXor;
                case TokenType::BitwiseNot:  // In binary context, ~ is XOR
                    return Precedence::BitwiseXor;
                case TokenType::BitwiseAnd:
                    return Precedence::BitwiseAnd;
                case TokenType::ShiftLeft:
                case TokenType::ShiftRight:
                    return Precedence::Shift;
                case TokenType::Concat:
                    return Precedence::Concat;
                case TokenType::Plus:
                case TokenType::Minus:
                    return Precedence::Term;
                case TokenType::Multiply:
                case TokenType::Divide:
                case TokenType::IntegerDivide:
                case TokenType::Modulo:
                    return Precedence::Factor;
                case TokenType::Power:
                    return Precedence::Power;
                case TokenType::LeftParen:
                case TokenType::LeftBracket:
                case TokenType::Dot:
                case TokenType::Colon:
                    return Precedence::Call;
                default:
                    return Precedence::None;
            }
        }

        bool is_binary_operator(TokenType type) noexcept {
            switch (type) {
                case TokenType::Plus:
                case TokenType::Minus:
                case TokenType::Multiply:
                case TokenType::Divide:
                case TokenType::IntegerDivide:
                case TokenType::Modulo:
                case TokenType::Power:
                case TokenType::Equal:
                case TokenType::NotEqual:
                case TokenType::Less:
                case TokenType::LessEqual:
                case TokenType::Greater:
                case TokenType::GreaterEqual:
                case TokenType::And:
                case TokenType::Or:
                case TokenType::Concat:
                case TokenType::BitwiseAnd:
                case TokenType::BitwiseOr:
                case TokenType::BitwiseXor:
                case TokenType::BitwiseNot:  // Can be binary XOR in context
                case TokenType::ShiftLeft:
                case TokenType::ShiftRight:
                    return true;
                default:
                    return false;
            }
        }

        bool is_unary_operator(TokenType type) noexcept {
            switch (type) {
                case TokenType::Minus:
                case TokenType::Not:
                case TokenType::Length:
                case TokenType::BitwiseNot:
                    return true;
                default:
                    return false;
            }
        }

        Optional<BinaryOpExpression::Operator> token_to_binary_op(TokenType type) noexcept {
            switch (type) {
                case TokenType::Plus:
                    return BinaryOpExpression::Operator::Add;
                case TokenType::Minus:
                    return BinaryOpExpression::Operator::Subtract;
                case TokenType::Multiply:
                    return BinaryOpExpression::Operator::Multiply;
                case TokenType::Divide:
                    return BinaryOpExpression::Operator::Divide;
                case TokenType::IntegerDivide:
                    return BinaryOpExpression::Operator::IntegerDivide;
                case TokenType::Modulo:
                    return BinaryOpExpression::Operator::Modulo;
                case TokenType::Power:
                    return BinaryOpExpression::Operator::Power;
                case TokenType::Equal:
                    return BinaryOpExpression::Operator::Equal;
                case TokenType::NotEqual:
                    return BinaryOpExpression::Operator::NotEqual;
                case TokenType::Less:
                    return BinaryOpExpression::Operator::Less;
                case TokenType::LessEqual:
                    return BinaryOpExpression::Operator::LessEqual;
                case TokenType::Greater:
                    return BinaryOpExpression::Operator::Greater;
                case TokenType::GreaterEqual:
                    return BinaryOpExpression::Operator::GreaterEqual;
                case TokenType::And:
                    return BinaryOpExpression::Operator::And;
                case TokenType::Or:
                    return BinaryOpExpression::Operator::Or;
                case TokenType::Concat:
                    return BinaryOpExpression::Operator::Concat;
                case TokenType::BitwiseAnd:
                    return BinaryOpExpression::Operator::BitwiseAnd;
                case TokenType::BitwiseOr:
                    return BinaryOpExpression::Operator::BitwiseOr;
                case TokenType::BitwiseXor:
                    return BinaryOpExpression::Operator::BitwiseXor;
                case TokenType::BitwiseNot:  // In binary context, ~ is XOR
                    return BinaryOpExpression::Operator::BitwiseXor;
                case TokenType::ShiftLeft:
                    return BinaryOpExpression::Operator::ShiftLeft;
                case TokenType::ShiftRight:
                    return BinaryOpExpression::Operator::ShiftRight;
                default:
                    return std::nullopt;
            }
        }

        Optional<UnaryOpExpression::Operator> token_to_unary_op(TokenType type) noexcept {
            switch (type) {
                case TokenType::Minus:
                    return UnaryOpExpression::Operator::Minus;
                case TokenType::Not:
                    return UnaryOpExpression::Operator::Not;
                case TokenType::Length:
                    return UnaryOpExpression::Operator::Length;
                case TokenType::BitwiseNot:
                    return UnaryOpExpression::Operator::BitwiseNot;
                default:
                    return std::nullopt;
            }
        }

        bool can_start_expression(TokenType type) noexcept {
            switch (type) {
                case TokenType::Number:
                case TokenType::String:
                case TokenType::True:
                case TokenType::False:
                case TokenType::Nil:
                case TokenType::Identifier:
                case TokenType::Ellipsis:
                case TokenType::LeftParen:
                case TokenType::LeftBrace:
                case TokenType::Function:
                case TokenType::Minus:
                case TokenType::Not:
                case TokenType::Length:
                case TokenType::BitwiseNot:
                    return true;
                default:
                    return false;
            }
        }

        bool can_start_statement(TokenType type) noexcept {
            switch (type) {
                case TokenType::Local:
                case TokenType::Function:
                case TokenType::If:
                case TokenType::While:
                case TokenType::For:
                case TokenType::Repeat:
                case TokenType::Do:
                case TokenType::Return:
                case TokenType::Break:
                case TokenType::Goto:
                case TokenType::DoubleColon:
                    return true;
                default:
                    return can_start_expression(type);
            }
        }

        bool is_statement_terminator(TokenType type) noexcept {
            switch (type) {
                case TokenType::Semicolon:
                case TokenType::Newline:
                case TokenType::EndOfFile:
                case TokenType::End:
                case TokenType::Else:
                case TokenType::Elseif:
                case TokenType::Until:
                    return true;
                default:
                    return false;
            }
        }

    }  // namespace parser_utils

    // AST builder implementations
    ExpressionPtr ASTBuilder::make_literal(LiteralExpression::Value value,
                                           SourceLocation location) {
        return std::make_unique<LiteralExpression>(std::move(value), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_identifier(String name, SourceLocation location) {
        return std::make_unique<IdentifierExpression>(std::move(name), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_binary_op(BinaryOpExpression::Operator op,
                                             ExpressionPtr left,
                                             ExpressionPtr right,
                                             SourceLocation location) {
        return std::make_unique<BinaryOpExpression>(
            op, std::move(left), std::move(right), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_unary_op(UnaryOpExpression::Operator op,
                                            ExpressionPtr operand,
                                            SourceLocation location) {
        return std::make_unique<UnaryOpExpression>(op, std::move(operand), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_function_call(ExpressionPtr function,
                                                 ExpressionList arguments,
                                                 SourceLocation location) {
        return std::make_unique<FunctionCallExpression>(
            std::move(function), std::move(arguments), std::move(location));
    }

    StatementPtr ASTBuilder::make_block(StatementList statements, SourceLocation location) {
        return std::make_unique<BlockStatement>(std::move(statements), std::move(location));
    }

    StatementPtr ASTBuilder::make_assignment(ExpressionList targets,
                                             ExpressionList values,
                                             SourceLocation location) {
        return std::make_unique<AssignmentStatement>(
            std::move(targets), std::move(values), std::move(location));
    }

    StatementPtr ASTBuilder::make_if(ExpressionPtr condition,
                                     StatementPtr then_body,
                                     std::vector<IfStatement::ElseIfClause> elseif_clauses,
                                     StatementPtr else_body,
                                     SourceLocation location) {
        return std::make_unique<IfStatement>(std::move(condition),
                                             std::move(then_body),
                                             std::move(elseif_clauses),
                                             std::move(else_body),
                                             std::move(location));
    }

    ProgramPtr ASTBuilder::make_program(StatementList statements, SourceLocation location) {
        return std::make_unique<Program>(std::move(statements), std::move(location));
    }

    // New AST builder methods
    ExpressionPtr ASTBuilder::make_method_call(ExpressionPtr object,
                                               String method_name,
                                               ExpressionList arguments,
                                               SourceLocation location) {
        return std::make_unique<MethodCallExpression>(
            std::move(object), std::move(method_name), std::move(arguments), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_table_access(ExpressionPtr table,
                                                ExpressionPtr key,
                                                bool is_dot_notation,
                                                SourceLocation location) {
        return std::make_unique<TableAccessExpression>(
            std::move(table), std::move(key), is_dot_notation, std::move(location));
    }

    ExpressionPtr ASTBuilder::make_table_constructor(TableConstructorExpression::FieldList fields,
                                                     SourceLocation location) {
        return std::make_unique<TableConstructorExpression>(std::move(fields), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_function_expression(FunctionExpression::ParameterList parameters,
                                                       StatementPtr body,
                                                       SourceLocation location) {
        return std::make_unique<FunctionExpression>(
            std::move(parameters), std::move(body), std::move(location));
    }

    ExpressionPtr ASTBuilder::make_vararg(SourceLocation location) {
        return std::make_unique<VarargExpression>(std::move(location));
    }

    ExpressionPtr ASTBuilder::make_parenthesized(ExpressionPtr expression,
                                                 SourceLocation location) {
        return std::make_unique<ParenthesizedExpression>(std::move(expression),
                                                         std::move(location));
    }

    StatementPtr ASTBuilder::make_local_declaration(std::vector<String> names,
                                                    ExpressionList values,
                                                    SourceLocation location) {
        return std::make_unique<LocalDeclarationStatement>(
            std::move(names), std::move(values), std::move(location));
    }

    StatementPtr ASTBuilder::make_function_declaration(ExpressionPtr name,
                                                       FunctionExpression::ParameterList parameters,
                                                       StatementPtr body,
                                                       bool is_local,
                                                       SourceLocation location) {
        return std::make_unique<FunctionDeclarationStatement>(
            std::move(name), std::move(parameters), std::move(body), is_local, std::move(location));
    }

    StatementPtr
    ASTBuilder::make_while(ExpressionPtr condition, StatementPtr body, SourceLocation location) {
        return std::make_unique<WhileStatement>(
            std::move(condition), std::move(body), std::move(location));
    }

    StatementPtr ASTBuilder::make_for_numeric(String variable,
                                              ExpressionPtr start,
                                              ExpressionPtr stop,
                                              ExpressionPtr step,
                                              StatementPtr body,
                                              SourceLocation location) {
        return std::make_unique<ForNumericStatement>(std::move(variable),
                                                     std::move(start),
                                                     std::move(stop),
                                                     std::move(step),
                                                     std::move(body),
                                                     std::move(location));
    }

    StatementPtr ASTBuilder::make_for_generic(std::vector<String> variables,
                                              ExpressionList expressions,
                                              StatementPtr body,
                                              SourceLocation location) {
        return std::make_unique<ForGenericStatement>(
            std::move(variables), std::move(expressions), std::move(body), std::move(location));
    }

    StatementPtr
    ASTBuilder::make_repeat(StatementPtr body, ExpressionPtr condition, SourceLocation location) {
        return std::make_unique<RepeatStatement>(
            std::move(body), std::move(condition), std::move(location));
    }

    StatementPtr ASTBuilder::make_do(StatementPtr body, SourceLocation location) {
        return std::make_unique<DoStatement>(std::move(body), std::move(location));
    }

    StatementPtr ASTBuilder::make_return(ExpressionList values, SourceLocation location) {
        return std::make_unique<ReturnStatement>(std::move(values), std::move(location));
    }

    StatementPtr ASTBuilder::make_break(SourceLocation location) {
        return std::make_unique<BreakStatement>(std::move(location));
    }

    StatementPtr ASTBuilder::make_goto(String label, SourceLocation location) {
        return std::make_unique<GotoStatement>(std::move(label), std::move(location));
    }

    StatementPtr ASTBuilder::make_label(String name, SourceLocation location) {
        return std::make_unique<LabelStatement>(std::move(name), std::move(location));
    }

    StatementPtr ASTBuilder::make_expression_statement(ExpressionPtr expression,
                                                       SourceLocation location) {
        return std::make_unique<ExpressionStatement>(std::move(expression), std::move(location));
    }

}  // namespace rangelua::frontend
