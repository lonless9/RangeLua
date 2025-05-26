/**
 * @file lexer.cpp
 * @brief Lexer implementation stub
 * @version 0.1.0
 */

#include <rangelua/frontend/lexer.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::frontend {

    // Token implementation
    bool Token::is_keyword() const noexcept {
        return type >= TokenType::And && type <= TokenType::While;
    }

    bool Token::is_operator() const noexcept {
        return type >= TokenType::Plus && type <= TokenType::ShiftRight;
    }

    bool Token::is_literal() const noexcept {
        return type >= TokenType::Number && type <= TokenType::Nil;
    }

    bool Token::is_delimiter() const noexcept {
        return type >= TokenType::LeftParen && type <= TokenType::Ellipsis;
    }

    String Token::to_string() const {
        return value;
    }

    // Lexer implementation
    class Lexer::Impl {
    public:
        explicit Impl(StringView source, String filename)
            : source_(source), filename_(std::move(filename)), position_(0) {}

        Token next_token() {
            // TODO: Implement tokenization
            if (position_ >= source_.size()) {
                return Token(TokenType::EndOfFile, "", current_location());
            }

            // Simple stub - just return EOF for now
            position_ = source_.size();
            return Token(TokenType::EndOfFile, "", current_location());
        }

        const Token& peek_token() {
            if (!peeked_token_.has_value()) {
                peeked_token_ = next_token();
            }
            return peeked_token_.value();
        }

        SourceLocation current_location() const noexcept {
            return SourceLocation{filename_, line_, column_};
        }

        bool at_end() const noexcept { return position_ >= source_.size(); }

        bool has_errors() const noexcept { return !errors_.empty(); }

        const std::vector<String>& errors() const noexcept { return errors_; }

    private:
        StringView source_;
        String filename_;
        Size position_;
        Size line_ = 1;
        Size column_ = 1;
        std::vector<String> errors_;
        Optional<Token> peeked_token_;
    };

    Lexer::Lexer(StringView source, String filename)
        : impl_(std::make_unique<Impl>(source, std::move(filename))) {}

    Lexer::Lexer(std::istream& input, String filename)
        : impl_(std::make_unique<Impl>("", std::move(filename))) {
        // TODO: Read from stream
    }

    Lexer::~Lexer() = default;

    Token Lexer::next_token() {
        return impl_->next_token();
    }

    const Token& Lexer::peek_token() {
        return impl_->peek_token();
    }

    SourceLocation Lexer::current_location() const noexcept {
        return impl_->current_location();
    }

    bool Lexer::at_end() const noexcept {
        return impl_->at_end();
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        while (!at_end()) {
            auto token = next_token();
            tokens.push_back(std::move(token));
            if (token.type == TokenType::EndOfFile) {
                break;
            }
        }
        return tokens;
    }

    bool Lexer::has_errors() const noexcept {
        return impl_->has_errors();
    }

    const std::vector<String>& Lexer::errors() const noexcept {
        return impl_->errors();
    }

    // Utility functions
    StringView token_type_to_string(TokenType type) noexcept {
        switch (type) {
            case TokenType::Number:
                return "Number";
            case TokenType::String:
                return "String";
            case TokenType::Boolean:
                return "Boolean";
            case TokenType::Nil:
                return "Nil";
            case TokenType::Identifier:
                return "Identifier";
            case TokenType::And:
                return "And";
            case TokenType::Break:
                return "Break";
            case TokenType::Do:
                return "Do";
            case TokenType::Else:
                return "Else";
            case TokenType::Elseif:
                return "Elseif";
            case TokenType::End:
                return "End";
            case TokenType::False:
                return "False";
            case TokenType::For:
                return "For";
            case TokenType::Function:
                return "Function";
            case TokenType::Goto:
                return "Goto";
            case TokenType::If:
                return "If";
            case TokenType::In:
                return "In";
            case TokenType::Local:
                return "Local";
            case TokenType::Not:
                return "Not";
            case TokenType::Or:
                return "Or";
            case TokenType::Repeat:
                return "Repeat";
            case TokenType::Return:
                return "Return";
            case TokenType::Then:
                return "Then";
            case TokenType::True:
                return "True";
            case TokenType::Until:
                return "Until";
            case TokenType::While:
                return "While";
            case TokenType::Plus:
                return "Plus";
            case TokenType::Minus:
                return "Minus";
            case TokenType::Multiply:
                return "Multiply";
            case TokenType::Divide:
                return "Divide";
            case TokenType::Modulo:
                return "Modulo";
            case TokenType::Power:
                return "Power";
            case TokenType::Equal:
                return "Equal";
            case TokenType::NotEqual:
                return "NotEqual";
            case TokenType::LessEqual:
                return "LessEqual";
            case TokenType::GreaterEqual:
                return "GreaterEqual";
            case TokenType::Less:
                return "Less";
            case TokenType::Greater:
                return "Greater";
            case TokenType::Assign:
                return "Assign";
            case TokenType::Concat:
                return "Concat";
            case TokenType::EndOfFile:
                return "EndOfFile";
            case TokenType::Invalid:
                return "Invalid";
            default:
                return "Unknown";
        }
    }

    Optional<TokenType> string_to_keyword(StringView str) noexcept {
        // TODO: Implement keyword lookup
        return std::nullopt;
    }

}  // namespace rangelua::frontend
