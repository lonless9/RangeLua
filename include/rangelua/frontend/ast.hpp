#pragma once

/**
 * @file ast.hpp
 * @brief Abstract Syntax Tree definitions with Visitor pattern support
 * @version 0.1.0
 */

#include <memory>
#include <variant>
#include <vector>

#include "../core/error.hpp"
#include "../core/types.hpp"
#include "lexer.hpp"

namespace rangelua::frontend {

    // Forward declarations
    class ASTVisitor;
    template <typename T>
    class ASTVisitorT;

    /**
     * @brief Base class for all AST nodes
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
            TableAccess,
            TableConstructor,

            // Statements
            Assignment,
            LocalDeclaration,
            FunctionDeclaration,
            IfStatement,
            WhileLoop,
            ForLoop,
            RepeatLoop,
            DoBlock,
            ReturnStatement,
            BreakStatement,
            GotoStatement,
            LabelStatement,

            // Special
            Block,
            Parameter,
            Field,
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

        template <typename T>
        void accept(ASTVisitorT<T>& visitor);

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
        [[nodiscard]] String to_string() const override;

    private:
        String name_;
    };

    /**
     * @brief Binary operation expression
     */
    class BinaryOpExpression : public Expression {
    public:
        enum class Operator : std::uint8_t {
            Add,
            Subtract,
            Multiply,
            Divide,
            Modulo,
            Power,
            Equal,
            NotEqual,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
            And,
            Or,
            Concat,
            BitwiseAnd,
            BitwiseOr,
            BitwiseXor,
            ShiftLeft,
            ShiftRight
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
        [[nodiscard]] String to_string() const override;

    private:
        Operator operator_;
        ExpressionPtr left_;
        ExpressionPtr right_;
    };

    /**
     * @brief Unary operation expression
     */
    class UnaryOpExpression : public Expression {
    public:
        enum class Operator : std::uint8_t { Minus, Not, Length, BitwiseNot };

        UnaryOpExpression(Operator op, ExpressionPtr operand, SourceLocation location = {}) noexcept
            : Expression(NodeType::UnaryOp, std::move(location)),
              operator_(op),
              operand_(std::move(operand)) {}

        [[nodiscard]] Operator operator_type() const noexcept { return operator_; }
        [[nodiscard]] const Expression& operand() const noexcept { return *operand_; }

        void accept(ASTVisitor& visitor) const override;
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
        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr function_;
        ExpressionList arguments_;
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
        [[nodiscard]] String to_string() const override;

    private:
        ExpressionPtr condition_;
        StatementPtr then_body_;
        std::vector<ElseIfClause> elseif_clauses_;
        StatementPtr else_body_;
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
        [[nodiscard]] String to_string() const override;

    private:
        StatementList statements_;
    };

    using ProgramPtr = UniquePtr<Program>;

    /**
     * @brief Visitor interface for AST traversal
     */
    class ASTVisitor {
    public:
        virtual ~ASTVisitor() = default;

        virtual void visit(const LiteralExpression& node) = 0;
        virtual void visit(const IdentifierExpression& node) = 0;
        virtual void visit(const BinaryOpExpression& node) = 0;
        virtual void visit(const UnaryOpExpression& node) = 0;
        virtual void visit(const FunctionCallExpression& node) = 0;
        virtual void visit(const BlockStatement& node) = 0;
        virtual void visit(const AssignmentStatement& node) = 0;
        virtual void visit(const IfStatement& node) = 0;
        virtual void visit(const Program& node) = 0;
    };

    /**
     * @brief Typed visitor template for return values
     */
    template <typename T>
    class ASTVisitorT {
    public:
        virtual ~ASTVisitorT() = default;

        virtual T visit(const LiteralExpression& node) = 0;
        virtual T visit(const IdentifierExpression& node) = 0;
        virtual T visit(const BinaryOpExpression& node) = 0;
        virtual T visit(const UnaryOpExpression& node) = 0;
        virtual T visit(const FunctionCallExpression& node) = 0;
        virtual T visit(const BlockStatement& node) = 0;
        virtual T visit(const AssignmentStatement& node) = 0;
        virtual T visit(const IfStatement& node) = 0;
        virtual T visit(const Program& node) = 0;
    };

}  // namespace rangelua::frontend

// Concept verification
// static_assert(rangelua::concepts::ASTNode<rangelua::frontend::ASTNode>);
