#pragma once

/**
 * @file lexer.hpp
 * @brief Lexical analyzer for Lua source code
 * @version 0.1.0
 */

#include <iostream>
#include <vector>

#include "../core/concepts.hpp"
#include "../core/types.hpp"

namespace rangelua::frontend {

    /**
     * @brief Token types recognized by the lexer
     */
    enum class TokenType : std::uint8_t {
        // Literals
        Number,
        String,
        Boolean,
        Nil,

        // Identifiers and keywords
        Identifier,

        // Keywords
        And,
        Break,
        Do,
        Else,
        Elseif,
        End,
        False,
        For,
        Function,
        Goto,
        If,
        In,
        Local,
        Not,
        Or,
        Repeat,
        Return,
        Then,
        True,
        Until,
        While,

        // Operators
        Plus,
        Minus,
        Multiply,
        Divide,
        Modulo,
        Power,
        Equal,
        NotEqual,
        LessEqual,
        GreaterEqual,
        Less,
        Greater,
        Assign,
        Concat,

        // Bitwise operators
        BitwiseAnd,
        BitwiseOr,
        BitwiseXor,
        BitwiseNot,
        ShiftLeft,
        ShiftRight,

        // Delimiters
        LeftParen,
        RightParen,
        LeftBrace,
        RightBrace,
        LeftBracket,
        RightBracket,
        Semicolon,
        Comma,
        Dot,
        Colon,
        DoubleColon,
        Ellipsis,

        // Special
        EndOfFile,
        Newline,
        Comment,

        // Error
        Invalid
    };

    /**
     * @brief Token structure containing type, value, and location information
     */
    struct Token {
        TokenType type = TokenType::Invalid;
        String value;
        SourceLocation location;

        // For numeric tokens
        Optional<Number> number_value;
        Optional<Int> integer_value;

        constexpr Token() noexcept = default;

        Token(TokenType t, String v, SourceLocation loc) noexcept
            : type(t), value(std::move(v)), location(std::move(loc)) {}

        Token(TokenType t, String v, SourceLocation loc, Number num) noexcept
            : type(t), value(std::move(v)), location(std::move(loc)), number_value(num) {}

        Token(TokenType t, String v, SourceLocation loc, Int integer) noexcept
            : type(t), value(std::move(v)), location(std::move(loc)), integer_value(integer) {}

        [[nodiscard]] bool is_keyword() const noexcept;
        [[nodiscard]] bool is_operator() const noexcept;
        [[nodiscard]] bool is_literal() const noexcept;
        [[nodiscard]] bool is_delimiter() const noexcept;

        [[nodiscard]] String to_string() const;
    };

    /**
     * @brief Lexical analyzer that converts source code into tokens
     */
    class Lexer {
    public:
        using Token = rangelua::frontend::Token;  // For concept compliance

        explicit Lexer(StringView source, String filename = "<input>");
        explicit Lexer(std::istream& input, String filename = "<stream>");

        ~Lexer();

        // Non-copyable, movable
        Lexer(const Lexer&) = delete;
        Lexer& operator=(const Lexer&) = delete;
        Lexer(Lexer&&) noexcept = default;
        Lexer& operator=(Lexer&&) noexcept = delete;

        /**
         * @brief Get the next token from the input
         * @return Next token
         */
        Token next_token();

        /**
         * @brief Peek at the next token without consuming it
         * @return Next token
         */
        [[nodiscard]] const Token& peek_token();

        /**
         * @brief Get current source location
         * @return Current location in source
         */
        [[nodiscard]] SourceLocation current_location() const noexcept;

        /**
         * @brief Check if end of input reached
         * @return true if at end of input
         */
        [[nodiscard]] bool at_end() const noexcept;

        /**
         * @brief Get all tokens from input
         * @return Vector of all tokens
         */
        std::vector<Token> tokenize();

        /**
         * @brief Check if lexer has encountered errors
         * @return true if errors occurred
         */
        [[nodiscard]] bool has_errors() const noexcept;

        /**
         * @brief Get list of lexer errors
         * @return Vector of error messages
         */
        [[nodiscard]] const std::vector<String>& errors() const noexcept;

    private:
        class Impl;
        UniquePtr<Impl> impl_;
    };

    /**
     * @brief Convert token type to string representation
     * @param type Token type
     * @return String representation
     */
    StringView token_type_to_string(TokenType type) noexcept;

    /**
     * @brief Check if string is a Lua keyword
     * @param str String to check
     * @return Token type if keyword, nullopt otherwise
     */
    Optional<TokenType> string_to_keyword(StringView str) noexcept;

    /**
     * @brief Token stream iterator for range-based loops
     */
    class TokenIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Token;
        using difference_type = std::ptrdiff_t;
        using pointer = const Token*;
        using reference = const Token&;

        explicit TokenIterator(Lexer& lexer) : lexer_(&lexer), current_token_(lexer.next_token()) {}
        TokenIterator() : lexer_(nullptr) {}  // End iterator

        reference operator*() const { return current_token_; }
        pointer operator->() const { return &current_token_; }

        TokenIterator& operator++() {
            if (lexer_ && current_token_.type != TokenType::EndOfFile) {
                current_token_ = lexer_->next_token();
            } else {
                lexer_ = nullptr;
            }
            return *this;
        }

        TokenIterator operator++(int) {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const TokenIterator& other) const { return lexer_ == other.lexer_; }

        bool operator!=(const TokenIterator& other) const { return !(*this == other); }

    private:
        Lexer* lexer_;
        Token current_token_;
    };

    /**
     * @brief Token range for range-based iteration
     */
    class TokenRange {
    public:
        explicit TokenRange(Lexer& lexer) : lexer_(lexer) {}

        TokenIterator begin() { return TokenIterator{lexer_}; }
        TokenIterator end() { return TokenIterator{}; }

    private:
        Lexer& lexer_;
    };

    /**
     * @brief Create token range from lexer
     * @param lexer Lexer instance
     * @return Token range
     */
    inline TokenRange make_token_range(Lexer& lexer) {
        return TokenRange(lexer);
    }

}  // namespace rangelua::frontend

// Concept verification (commented out until concepts are properly implemented)
// static_assert(rangelua::concepts::Lexer<rangelua::frontend::Lexer>);
