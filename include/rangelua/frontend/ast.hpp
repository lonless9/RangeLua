#pragma once

/**
 * @file ast.hpp
 * @brief Abstract Syntax Tree definitions with Visitor pattern support for Lua 5.5
 * @version 0.1.0
 */

#include <concepts>
#include <memory>
#include <ranges>
#include <variant>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/error.hpp"
#include "../core/types.hpp"
#include "lexer.hpp"

namespace rangelua::frontend {

    // Forward declarations
    class ASTVisitor;
    template <typename T>
    class ASTVisitorT;

    /**
     * @brief Base class for all AST nodes with modern C++20 features
     */
    class ASTNode {
    public:
        enum class NodeType : std::uint8_t {
            // Expressions
            Literal,
            Identifier,
            BinaryOp,
            UnaryOp,
            FunctionCall,
            MethodCall,
            TableAccess,
            TableConstructor,
            FunctionExpression,
            Vararg,
            Parenthesized,
            Index,

            // Statements
            Assignment,
            LocalDeclaration,
            FunctionDeclaration,
            IfStatement,
            WhileStatement,
            ForNumericStatement,
            ForGenericStatement,
            RepeatStatement,
            DoStatement,
            ReturnStatement,
            BreakStatement,
            GotoStatement,
            LabelStatement,
            ExpressionStatement,

            // Special nodes
            Block,
            Parameter,
            Field,
            ElseIfClause,
            Program
        };

        explicit ASTNode(NodeType type, SourceLocation location = {}) noexcept
            : type_(type), location_(std::move(location)) {}

        virtual ~ASTNode() = default;

        // Non-copyable, movable
        ASTNode(const ASTNode&) = delete;
        ASTNode& operator=(const ASTNode&) = delete;
        ASTNode(ASTNode&&) noexcept = default;
        ASTNode& operator=(ASTNode&&) noexcept = default;

        [[nodiscard]] NodeType type() const noexcept { return type_; }
        [[nodiscard]] const SourceLocation& location() const noexcept { return location_; }

        virtual void accept(ASTVisitor& visitor) const = 0;

        [[nodiscard]] virtual String to_string() const = 0;

    protected:
        NodeType type_;
        SourceLocation location_;
    };

    using ASTNodePtr = UniquePtr<ASTNode>;
    using ASTNodeList = std::vector<ASTNodePtr>;

    /**
     * @brief Expression base class
     */
    class Expression : public ASTNode {
    public:
        explicit Expression(NodeType type, SourceLocation location = {}) noexcept
            : ASTNode(type, std::move(location)) {}
    };

    using ExpressionPtr = UniquePtr<Expression>;
    using ExpressionList = std::vector<ExpressionPtr>;

    /**
     * @brief Statement base class
     */
    class Statement : public ASTNode {
    public:
        explicit Statement(NodeType type, SourceLocation location = {}) noexcept
            : ASTNode(type, std::move(location)) {}
    };

    using StatementPtr = UniquePtr<Statement>;
    using StatementList = std::vector<StatementPtr>;

    /**
     * @brief Literal expression (numbers, strings, booleans, nil)
     */
    class LiteralExpression : public Expression {
    public:
        using Value = Variant<Number, Int, String, bool, std::monostate>;  // monostate for nil

        explicit LiteralExpression(Value value, SourceLocation location = {}) noexcept
            : Expression(NodeType::Literal, std::move(location)), value_(std::move(value)) {}

        [[nodiscard]] const Value& value() const noexcept { return value_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        Value value_;
    };

    /**
     * @brief Identifier expression
     */
    class IdentifierExpression : public Expression {
    public:
        explicit IdentifierExpression(String name, SourceLocation location = {}) noexcept
            : Expression(NodeType::Identifier, std::move(location)), name_(std::move(name)) {}

        [[nodiscard]] const String& name() const noexcept { return name_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        String name_;
    };

    /**
     * @brief Binary operation expression with complete Lua 5.5 operator support
     */
    class BinaryOpExpression : public Expression {
    public:
        enum class Operator : std::uint8_t {
            // Arithmetic operators
            Add,            // +
            Subtract,       // -
            Multiply,       // *
            Divide,         // /
            IntegerDivide,  // //
            Modulo,         // %
            Power,          // ^

            // Relational operators
            Equal,         // ==
            NotEqual,      // ~=
            Less,          // <
            LessEqual,     // <=
            Greater,       // >
            GreaterEqual,  // >=

            // Logical operators
            And,  // and
            Or,   // or

            // String operator
            Concat,  // ..

            // Bitwise operators (Lua 5.3+)
            BitwiseAnd,  // &
            BitwiseOr,   // |
            BitwiseXor,  // ~
            ShiftLeft,   // <<
            ShiftRight   // >>
        };

        BinaryOpExpression(Operator op,
                           ExpressionPtr left,
                           ExpressionPtr right,
                           SourceLocation location = {}) noexcept
            : Expression(NodeType::BinaryOp, std::move(location)),
              operator_(op),
              left_(std::move(left)),
              right_(std::move(right)) {}

        [[nodiscard]] Operator operator_type() const noexcept { return operator_; }
        [[nodiscard]] const Expression& left() const noexcept { return *left_; }
        [[nodiscard]] const Expression& right() const noexcept { return *right_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        Operator operator_;
        ExpressionPtr left_;
        ExpressionPtr right_;
    };

    /**
     * @brief Unary operation expression with complete Lua 5.5 operator support
     */
    class UnaryOpExpression : public Expression {
    public:
        enum class Operator : std::uint8_t {
            Minus,      // - (unary minus)
            Not,        // not (logical not)
            Length,     // # (length operator)
            BitwiseNot  // ~ (bitwise not, Lua 5.3+)
        };

        UnaryOpExpression(Operator op, ExpressionPtr operand, SourceLocation location = {}) noexcept
            : Expression(NodeType::UnaryOp, std::move(location)),
              operator_(op),
              operand_(std::move(operand)) {}

        [[nodiscard]] Operator operator_type() const noexcept { return operator_; }
        [[nodiscard]] const Expression& operand() const noexcept { return *operand_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        Operator operator_;
        ExpressionPtr operand_;
    };

    /**
     * @brief Function call expression
     */
    class FunctionCallExpression : public Expression {
    public:
        FunctionCallExpression(ExpressionPtr function,
                               ExpressionList arguments,
                               SourceLocation location = {}) noexcept
            : Expression(NodeType::FunctionCall, std::move(location)),
              function_(std::move(function)),
              arguments_(std::move(arguments)) {}

        [[nodiscard]] const Expression& function() const noexcept { return *function_; }
        [[nodiscard]] const ExpressionList& arguments() const noexcept { return arguments_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr function_;
        ExpressionList arguments_;
    };

    /**
     * @brief Method call expression (obj:method(args))
     */
    class MethodCallExpression : public Expression {
    public:
        MethodCallExpression(ExpressionPtr object,
                             String method_name,
                             ExpressionList arguments,
                             SourceLocation location = {}) noexcept
            : Expression(NodeType::MethodCall, std::move(location)),
              object_(std::move(object)),
              method_name_(std::move(method_name)),
              arguments_(std::move(arguments)) {}

        [[nodiscard]] const Expression& object() const noexcept { return *object_; }
        [[nodiscard]] const String& method_name() const noexcept { return method_name_; }
        [[nodiscard]] const ExpressionList& arguments() const noexcept { return arguments_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr object_;
        String method_name_;
        ExpressionList arguments_;
    };

    /**
     * @brief Table access expression (table[key] or table.field)
     */
    class TableAccessExpression : public Expression {
    public:
        TableAccessExpression(ExpressionPtr table,
                              ExpressionPtr key,
                              bool is_dot_notation = false,
                              SourceLocation location = {}) noexcept
            : Expression(NodeType::TableAccess, std::move(location)),
              table_(std::move(table)),
              key_(std::move(key)),
              is_dot_notation_(is_dot_notation) {}

        [[nodiscard]] const Expression& table() const noexcept { return *table_; }
        [[nodiscard]] const Expression& key() const noexcept { return *key_; }
        [[nodiscard]] bool is_dot_notation() const noexcept { return is_dot_notation_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr table_;
        ExpressionPtr key_;
        bool is_dot_notation_;
    };

    /**
     * @brief Table constructor expression ({...})
     */
    class TableConstructorExpression : public Expression {
    public:
        struct Field {
            enum class Type { Array, Record, List };
            Type type;
            ExpressionPtr key;  // nullptr for array/list fields
            ExpressionPtr value;

            Field(Type t, ExpressionPtr k, ExpressionPtr v) noexcept
                : type(t), key(std::move(k)), value(std::move(v)) {}
        };

        using FieldList = std::vector<Field>;

        explicit TableConstructorExpression(FieldList fields, SourceLocation location = {}) noexcept
            : Expression(NodeType::TableConstructor, std::move(location)),
              fields_(std::move(fields)) {}

        [[nodiscard]] const FieldList& fields() const noexcept { return fields_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        FieldList fields_;
    };

    /**
     * @brief Function expression (function(...) ... end)
     */
    class FunctionExpression : public Expression {
    public:
        struct Parameter {
            String name;
            bool is_vararg = false;

            explicit Parameter(String n, bool vararg = false) noexcept
                : name(std::move(n)), is_vararg(vararg) {}
        };

        using ParameterList = std::vector<Parameter>;

        FunctionExpression(ParameterList parameters,
                           StatementPtr body,
                           SourceLocation location = {}) noexcept
            : Expression(NodeType::FunctionExpression, std::move(location)),
              parameters_(std::move(parameters)),
              body_(std::move(body)) {}

        [[nodiscard]] const ParameterList& parameters() const noexcept { return parameters_; }
        [[nodiscard]] const Statement& body() const noexcept { return *body_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ParameterList parameters_;
        StatementPtr body_;
    };

    /**
     * @brief Vararg expression (...)
     */
    class VarargExpression : public Expression {
    public:
        explicit VarargExpression(SourceLocation location = {}) noexcept
            : Expression(NodeType::Vararg, std::move(location)) {}

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;
    };

    /**
     * @brief Parenthesized expression ((expr))
     */
    class ParenthesizedExpression : public Expression {
    public:
        explicit ParenthesizedExpression(ExpressionPtr expression,
                                         SourceLocation location = {}) noexcept
            : Expression(NodeType::Parenthesized, std::move(location)),
              expression_(std::move(expression)) {}

        [[nodiscard]] const Expression& expression() const noexcept { return *expression_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr expression_;
    };

    /**
     * @brief Block statement containing multiple statements
     */
    class BlockStatement : public Statement {
    public:
        explicit BlockStatement(StatementList statements, SourceLocation location = {}) noexcept
            : Statement(NodeType::Block, std::move(location)), statements_(std::move(statements)) {}

        [[nodiscard]] const StatementList& statements() const noexcept { return statements_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        StatementList statements_;
    };

    /**
     * @brief Assignment statement
     */
    class AssignmentStatement : public Statement {
    public:
        AssignmentStatement(ExpressionList targets,
                            ExpressionList values,
                            SourceLocation location = {}) noexcept
            : Statement(NodeType::Assignment, std::move(location)),
              targets_(std::move(targets)),
              values_(std::move(values)) {}

        [[nodiscard]] const ExpressionList& targets() const noexcept { return targets_; }
        [[nodiscard]] const ExpressionList& values() const noexcept { return values_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionList targets_;
        ExpressionList values_;
    };

    /**
     * @brief If statement with optional elseif and else clauses
     */
    class IfStatement : public Statement {
    public:
        struct ElseIfClause {
            ExpressionPtr condition;
            StatementPtr body;
        };

        IfStatement(ExpressionPtr condition,
                    StatementPtr then_body,
                    std::vector<ElseIfClause> elseif_clauses = {},
                    StatementPtr else_body = nullptr,
                    SourceLocation location = {}) noexcept
            : Statement(NodeType::IfStatement, std::move(location)),
              condition_(std::move(condition)),
              then_body_(std::move(then_body)),
              elseif_clauses_(std::move(elseif_clauses)),
              else_body_(std::move(else_body)) {}

        [[nodiscard]] const Expression& condition() const noexcept { return *condition_; }
        [[nodiscard]] const Statement& then_body() const noexcept { return *then_body_; }
        [[nodiscard]] const std::vector<ElseIfClause>& elseif_clauses() const noexcept {
            return elseif_clauses_;
        }
        [[nodiscard]] const Statement* else_body() const noexcept { return else_body_.get(); }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr condition_;
        StatementPtr then_body_;
        std::vector<ElseIfClause> elseif_clauses_;
        StatementPtr else_body_;
    };

    /**
     * @brief Local variable declaration statement
     */
    class LocalDeclarationStatement : public Statement {
    public:
        LocalDeclarationStatement(std::vector<String> names,
                                  ExpressionList values = {},
                                  SourceLocation location = {}) noexcept
            : Statement(NodeType::LocalDeclaration, std::move(location)),
              names_(std::move(names)),
              values_(std::move(values)) {}

        [[nodiscard]] const std::vector<String>& names() const noexcept { return names_; }
        [[nodiscard]] const ExpressionList& values() const noexcept { return values_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        std::vector<String> names_;
        ExpressionList values_;
    };

    /**
     * @brief Function declaration statement
     */
    class FunctionDeclarationStatement : public Statement {
    public:
        FunctionDeclarationStatement(ExpressionPtr name,
                                     FunctionExpression::ParameterList parameters,
                                     StatementPtr body,
                                     bool is_local = false,
                                     SourceLocation location = {}) noexcept
            : Statement(NodeType::FunctionDeclaration, std::move(location)),
              name_(std::move(name)),
              parameters_(std::move(parameters)),
              body_(std::move(body)),
              is_local_(is_local) {}

        [[nodiscard]] const Expression& name() const noexcept { return *name_; }
        [[nodiscard]] const FunctionExpression::ParameterList& parameters() const noexcept {
            return parameters_;
        }
        [[nodiscard]] const Statement& body() const noexcept { return *body_; }
        [[nodiscard]] bool is_local() const noexcept { return is_local_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr name_;
        FunctionExpression::ParameterList parameters_;
        StatementPtr body_;
        bool is_local_;
    };

    /**
     * @brief While loop statement
     */
    class WhileStatement : public Statement {
    public:
        WhileStatement(ExpressionPtr condition,
                       StatementPtr body,
                       SourceLocation location = {}) noexcept
            : Statement(NodeType::WhileStatement, std::move(location)),
              condition_(std::move(condition)),
              body_(std::move(body)) {}

        [[nodiscard]] const Expression& condition() const noexcept { return *condition_; }
        [[nodiscard]] const Statement& body() const noexcept { return *body_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr condition_;
        StatementPtr body_;
    };

    /**
     * @brief Numeric for loop statement (for i = start, stop, step do ... end)
     */
    class ForNumericStatement : public Statement {
    public:
        ForNumericStatement(String variable,
                            ExpressionPtr start,
                            ExpressionPtr stop,
                            ExpressionPtr step,
                            StatementPtr body,
                            SourceLocation location = {}) noexcept
            : Statement(NodeType::ForNumericStatement, std::move(location)),
              variable_(std::move(variable)),
              start_(std::move(start)),
              stop_(std::move(stop)),
              step_(std::move(step)),
              body_(std::move(body)) {}

        [[nodiscard]] const String& variable() const noexcept { return variable_; }
        [[nodiscard]] const Expression& start() const noexcept { return *start_; }
        [[nodiscard]] const Expression& stop() const noexcept { return *stop_; }
        [[nodiscard]] const Expression* step() const noexcept { return step_.get(); }
        [[nodiscard]] const Statement& body() const noexcept { return *body_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        String variable_;
        ExpressionPtr start_;
        ExpressionPtr stop_;
        ExpressionPtr step_;  // nullptr if not specified (defaults to 1)
        StatementPtr body_;
    };

    /**
     * @brief Generic for loop statement (for vars in explist do ... end)
     */
    class ForGenericStatement : public Statement {
    public:
        ForGenericStatement(std::vector<String> variables,
                            ExpressionList expressions,
                            StatementPtr body,
                            SourceLocation location = {}) noexcept
            : Statement(NodeType::ForGenericStatement, std::move(location)),
              variables_(std::move(variables)),
              expressions_(std::move(expressions)),
              body_(std::move(body)) {}

        [[nodiscard]] const std::vector<String>& variables() const noexcept { return variables_; }
        [[nodiscard]] const ExpressionList& expressions() const noexcept { return expressions_; }
        [[nodiscard]] const Statement& body() const noexcept { return *body_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        std::vector<String> variables_;
        ExpressionList expressions_;
        StatementPtr body_;
    };

    /**
     * @brief Repeat-until loop statement
     */
    class RepeatStatement : public Statement {
    public:
        RepeatStatement(StatementPtr body,
                        ExpressionPtr condition,
                        SourceLocation location = {}) noexcept
            : Statement(NodeType::RepeatStatement, std::move(location)),
              body_(std::move(body)),
              condition_(std::move(condition)) {}

        [[nodiscard]] const Statement& body() const noexcept { return *body_; }
        [[nodiscard]] const Expression& condition() const noexcept { return *condition_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        StatementPtr body_;
        ExpressionPtr condition_;
    };

    /**
     * @brief Do-end block statement
     */
    class DoStatement : public Statement {
    public:
        explicit DoStatement(StatementPtr body, SourceLocation location = {}) noexcept
            : Statement(NodeType::DoStatement, std::move(location)), body_(std::move(body)) {}

        [[nodiscard]] const Statement& body() const noexcept { return *body_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        StatementPtr body_;
    };

    /**
     * @brief Return statement
     */
    class ReturnStatement : public Statement {
    public:
        explicit ReturnStatement(ExpressionList values = {}, SourceLocation location = {}) noexcept
            : Statement(NodeType::ReturnStatement, std::move(location)),
              values_(std::move(values)) {}

        [[nodiscard]] const ExpressionList& values() const noexcept { return values_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionList values_;
    };

    /**
     * @brief Break statement
     */
    class BreakStatement : public Statement {
    public:
        explicit BreakStatement(SourceLocation location = {}) noexcept
            : Statement(NodeType::BreakStatement, std::move(location)) {}

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;
    };

    /**
     * @brief Goto statement
     */
    class GotoStatement : public Statement {
    public:
        explicit GotoStatement(String label, SourceLocation location = {}) noexcept
            : Statement(NodeType::GotoStatement, std::move(location)), label_(std::move(label)) {}

        [[nodiscard]] const String& label() const noexcept { return label_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        String label_;
    };

    /**
     * @brief Label statement
     */
    class LabelStatement : public Statement {
    public:
        explicit LabelStatement(String name, SourceLocation location = {}) noexcept
            : Statement(NodeType::LabelStatement, std::move(location)), name_(std::move(name)) {}

        [[nodiscard]] const String& name() const noexcept { return name_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        String name_;
    };

    /**
     * @brief Expression statement (for function calls used as statements)
     */
    class ExpressionStatement : public Statement {
    public:
        explicit ExpressionStatement(ExpressionPtr expression,
                                     SourceLocation location = {}) noexcept
            : Statement(NodeType::ExpressionStatement, std::move(location)),
              expression_(std::move(expression)) {}

        [[nodiscard]] const Expression& expression() const noexcept { return *expression_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr expression_;
    };

    /**
     * @brief Program root node
     */
    class Program : public ASTNode {
    public:
        explicit Program(StatementList statements, SourceLocation location = {}) noexcept
            : ASTNode(NodeType::Program, std::move(location)), statements_(std::move(statements)) {}

        [[nodiscard]] const StatementList& statements() const noexcept { return statements_; }

        void accept(ASTVisitor& visitor) const override;

        template <typename T>
        T accept(ASTVisitorT<T>& visitor) const {
            return visitor.visit(*this);
        }

        [[nodiscard]] String to_string() const override;

    private:
        StatementList statements_;
    };

    using ProgramPtr = UniquePtr<Program>;

    /**
     * @brief Visitor interface for AST traversal with complete Lua 5.5 support
     */
    class ASTVisitor {
    public:
        virtual ~ASTVisitor() = default;

        // Expression visitors
        virtual void visit(const LiteralExpression& node) = 0;
        virtual void visit(const IdentifierExpression& node) = 0;
        virtual void visit(const BinaryOpExpression& node) = 0;
        virtual void visit(const UnaryOpExpression& node) = 0;
        virtual void visit(const FunctionCallExpression& node) = 0;
        virtual void visit(const MethodCallExpression& node) = 0;
        virtual void visit(const TableAccessExpression& node) = 0;
        virtual void visit(const TableConstructorExpression& node) = 0;
        virtual void visit(const FunctionExpression& node) = 0;
        virtual void visit(const VarargExpression& node) = 0;
        virtual void visit(const ParenthesizedExpression& node) = 0;

        // Statement visitors
        virtual void visit(const BlockStatement& node) = 0;
        virtual void visit(const AssignmentStatement& node) = 0;
        virtual void visit(const LocalDeclarationStatement& node) = 0;
        virtual void visit(const FunctionDeclarationStatement& node) = 0;
        virtual void visit(const IfStatement& node) = 0;
        virtual void visit(const WhileStatement& node) = 0;
        virtual void visit(const ForNumericStatement& node) = 0;
        virtual void visit(const ForGenericStatement& node) = 0;
        virtual void visit(const RepeatStatement& node) = 0;
        virtual void visit(const DoStatement& node) = 0;
        virtual void visit(const ReturnStatement& node) = 0;
        virtual void visit(const BreakStatement& node) = 0;
        virtual void visit(const GotoStatement& node) = 0;
        virtual void visit(const LabelStatement& node) = 0;
        virtual void visit(const ExpressionStatement& node) = 0;

        // Program visitor
        virtual void visit(const Program& node) = 0;

    public:
        // Make visitor non-copyable but movable
        ASTVisitor() = default;
        ASTVisitor(const ASTVisitor&) = delete;
        ASTVisitor& operator=(const ASTVisitor&) = delete;
        ASTVisitor(ASTVisitor&&) = default;
        ASTVisitor& operator=(ASTVisitor&&) = default;
    };

    /**
     * @brief Typed visitor template for return values with complete Lua 5.5 support
     */
    template <typename T>
    class ASTVisitorT {
    public:
        virtual ~ASTVisitorT() = default;

        // Expression visitors
        virtual T visit(const LiteralExpression& node) = 0;
        virtual T visit(const IdentifierExpression& node) = 0;
        virtual T visit(const BinaryOpExpression& node) = 0;
        virtual T visit(const UnaryOpExpression& node) = 0;
        virtual T visit(const FunctionCallExpression& node) = 0;
        virtual T visit(const MethodCallExpression& node) = 0;
        virtual T visit(const TableAccessExpression& node) = 0;
        virtual T visit(const TableConstructorExpression& node) = 0;
        virtual T visit(const FunctionExpression& node) = 0;
        virtual T visit(const VarargExpression& node) = 0;
        virtual T visit(const ParenthesizedExpression& node) = 0;

        // Statement visitors
        virtual T visit(const BlockStatement& node) = 0;
        virtual T visit(const AssignmentStatement& node) = 0;
        virtual T visit(const LocalDeclarationStatement& node) = 0;
        virtual T visit(const FunctionDeclarationStatement& node) = 0;
        virtual T visit(const IfStatement& node) = 0;
        virtual T visit(const WhileStatement& node) = 0;
        virtual T visit(const ForNumericStatement& node) = 0;
        virtual T visit(const ForGenericStatement& node) = 0;
        virtual T visit(const RepeatStatement& node) = 0;
        virtual T visit(const DoStatement& node) = 0;
        virtual T visit(const ReturnStatement& node) = 0;
        virtual T visit(const BreakStatement& node) = 0;
        virtual T visit(const GotoStatement& node) = 0;
        virtual T visit(const LabelStatement& node) = 0;
        virtual T visit(const ExpressionStatement& node) = 0;

        // Program visitor
        virtual T visit(const Program& node) = 0;

    public:
        // Make visitor non-copyable but movable
        ASTVisitorT() = default;
        ASTVisitorT(const ASTVisitorT&) = delete;
        ASTVisitorT& operator=(const ASTVisitorT&) = delete;
        ASTVisitorT(ASTVisitorT&&) = default;
        ASTVisitorT& operator=(ASTVisitorT&&) = default;
    };

    // Utility type aliases for convenience
    using TableFieldList = TableConstructorExpression::FieldList;
    using ParameterList = FunctionExpression::ParameterList;
    using ElseIfClauseList = std::vector<IfStatement::ElseIfClause>;

    // AST node variant for generic processing
    using AnyExpression = Variant<LiteralExpression,
                                  IdentifierExpression,
                                  BinaryOpExpression,
                                  UnaryOpExpression,
                                  FunctionCallExpression,
                                  MethodCallExpression,
                                  TableAccessExpression,
                                  TableConstructorExpression,
                                  FunctionExpression,
                                  VarargExpression,
                                  ParenthesizedExpression>;

    using AnyStatement = Variant<BlockStatement,
                                 AssignmentStatement,
                                 LocalDeclarationStatement,
                                 FunctionDeclarationStatement,
                                 IfStatement,
                                 WhileStatement,
                                 ForNumericStatement,
                                 ForGenericStatement,
                                 RepeatStatement,
                                 DoStatement,
                                 ReturnStatement,
                                 BreakStatement,
                                 GotoStatement,
                                 LabelStatement,
                                 ExpressionStatement>;

}  // namespace rangelua::frontend

// Concept verification for modern C++20 compliance
static_assert(rangelua::concepts::ASTNode<rangelua::frontend::ASTNode>);
static_assert(rangelua::concepts::BasicASTNode<rangelua::frontend::Expression>);
static_assert(rangelua::concepts::BasicASTNode<rangelua::frontend::Statement>);

// Verify visitor pattern compliance
static_assert(rangelua::concepts::Visitable<rangelua::frontend::LiteralExpression,
                                            rangelua::frontend::ASTVisitor>);
static_assert(rangelua::concepts::SingleNodeVisitor<rangelua::frontend::ASTVisitor,
                                                    rangelua::frontend::LiteralExpression>);
