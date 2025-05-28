/**
 * @file lexer.cpp
 * @brief Comprehensive lexer implementation for Lua 5.5 compatibility
 * @version 0.1.0
 */

#include <rangelua/frontend/lexer.hpp>
#include <rangelua/utils/logger.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <unordered_map>

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
        std::ostringstream oss;
        oss << token_type_to_string(type);
        if (!value.empty()) {
            oss << "('" << value << "')";
        }
        if (number_value.has_value()) {
            oss << " [num: " << number_value.value() << "]";
        }
        if (integer_value.has_value()) {
            oss << " [int: " << integer_value.value() << "]";
        }
        return oss.str();
    }

    // Character classification helpers
    namespace {
        constexpr bool is_alpha(char c) noexcept {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
        }

        constexpr bool is_digit(char c) noexcept {
            return c >= '0' && c <= '9';
        }

        constexpr bool is_alnum(char c) noexcept {
            return is_alpha(c) || is_digit(c);
        }

        constexpr bool is_hex_digit(char c) noexcept {
            return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        }

        constexpr bool is_space(char c) noexcept {
            return c == ' ' || c == '\t' || c == '\v' || c == '\f';
        }

        constexpr bool is_newline(char c) noexcept {
            return c == '\n' || c == '\r';
        }

        constexpr int hex_value(char c) noexcept {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }
    }  // namespace

    // Lexer implementation
    class Lexer::Impl {
    public:
        explicit Impl(StringView source, String filename)
            : source_(source), filename_(std::move(filename)), position_(0) {}

        Token next_token() {
            if (peeked_token_.has_value()) {
                auto token = std::move(peeked_token_.value());
                peeked_token_.reset();
                utils::loggers::lexer()->debug("Returning peeked token: {}", token.to_string());
                return token;
            }

            skip_whitespace_and_comments();

            if (at_end()) {
                utils::loggers::lexer()->debug("Reached end of file");
                return {TokenType::EndOfFile, "", current_location()};
            }

            const auto start_location = current_location();
            const char current_char = current();

            utils::loggers::lexer()->debug("Tokenizing character '{}' at {}:{}",
                                           current_char,
                                           start_location.line_,
                                           start_location.column_);

            Token token;
            // Handle different token types
            if (is_alpha(current_char)) {
                token = read_identifier_or_keyword(start_location);
            } else if (is_digit(current_char)) {
                token = read_number(start_location);
            } else if (current_char == '.' && is_digit(peek())) {
                token = read_number(start_location);
            } else if (current_char == '"' || current_char == '\'') {
                token = read_string(start_location);
            } else if (current_char == '[') {
                // Check for long string
                auto sep_level = check_long_string_separator();
                if (sep_level >= 0) {
                    token = read_long_string(start_location, sep_level);
                } else {
                    token = read_operator_or_delimiter(start_location);
                }
            } else {
                // Single and multi-character operators and delimiters
                token = read_operator_or_delimiter(start_location);
            }

            utils::loggers::lexer()->debug("Generated token: {}", token.to_string());
            return token;
        }

        const Token& peek_token() {
            if (!peeked_token_.has_value()) {
                peeked_token_ = next_token();
            }
            return peeked_token_.value();
        }

        [[nodiscard]] SourceLocation current_location() const noexcept {
            return SourceLocation{filename_, line_, column_};
        }

        [[nodiscard]] bool at_end() const noexcept { return position_ >= source_.size(); }

        [[nodiscard]] bool has_errors() const noexcept { return !errors_.empty(); }

        [[nodiscard]] const std::vector<String>& errors() const noexcept { return errors_; }

    private:
        // Character access and movement
        [[nodiscard]] char current() const noexcept { return at_end() ? '\0' : source_[position_]; }

        [[nodiscard]] char peek(Size offset = 1) const noexcept {
            const auto pos = position_ + offset;
            return pos >= source_.size() ? '\0' : source_[pos];
        }

        void advance() noexcept {
            if (!at_end()) {
                if (current() == '\n') {
                    ++line_;
                    column_ = 1;
                } else {
                    ++column_;
                }
                ++position_;
            }
        }

        char advance_and_return() noexcept {
            const char c = current();
            advance();
            return c;
        }

        // Error reporting
        void report_error(const String& message) {
            std::ostringstream oss;
            oss << filename_ << ":" << line_ << ":" << column_ << ": " << message;
            errors_.push_back(oss.str());
        }

        StringView source_;
        String filename_;
        Size position_ = 0;
        Size line_ = 1;
        Size column_ = 1;
        std::vector<String> errors_;
        Optional<Token> peeked_token_;

        // Token reading method implementations
        void skip_whitespace_and_comments() {
            while (!at_end()) {
                const char c = current();

                if (is_space(c)) {
                    advance();
                } else if (is_newline(c)) {
                    advance();
                    // Handle \r\n sequence
                    if (c == '\r' && current() == '\n') {
                        advance();
                    }
                } else if (c == '-' && peek() == '-') {
                    // Single-line comment
                    advance();  // skip first '-'
                    advance();  // skip second '-'

                    // Check for long comment
                    if (current() == '[') {
                        auto sep_level = check_long_string_separator();
                        if (sep_level >= 0) {
                            skip_long_comment(sep_level);
                            continue;
                        }
                    }

                    // Skip to end of line
                    while (!at_end() && !is_newline(current())) {
                        advance();
                    }
                } else {
                    break;
                }
            }
        }

        Token read_identifier_or_keyword(const SourceLocation& start_location) {
            String identifier;

            while (!at_end() && is_alnum(current())) {
                identifier += advance_and_return();
            }

            // Check if it's a keyword
            if (auto keyword_type = string_to_keyword(identifier)) {
                return {keyword_type.value(), identifier, start_location};
            }

            return {TokenType::Identifier, identifier, start_location};
        }

        Token read_number(const SourceLocation& start_location) {
            String number_str;
            bool is_float = false;
            bool is_hex = false;

            // Check if number starts with a dot
            if (current() == '.') {
                is_float = true;
                number_str += advance_and_return();  // '.'

                // Read fractional part
                while (!at_end() && is_digit(current())) {
                    number_str += advance_and_return();
                }
            } else {
                // Check for hexadecimal prefix
                if (current() == '0' && (peek() == 'x' || peek() == 'X')) {
                    is_hex = true;
                    number_str += advance_and_return();  // '0'
                    number_str += advance_and_return();  // 'x' or 'X'

                    if (!is_hex_digit(current())) {
                        report_error("malformed hexadecimal number");
                        return {TokenType::Invalid, number_str, start_location};
                    }
                }

                // Read integer part
                while (!at_end() && (is_hex ? is_hex_digit(current()) : is_digit(current()))) {
                    number_str += advance_and_return();
                }
            }

            // Check for decimal point (only for decimal numbers and if we haven't already processed
            // it)
            if (!is_hex && !is_float && current() == '.') {
                // Look ahead to ensure it's not ".." (concat operator)
                if (peek() != '.') {
                    is_float = true;
                    number_str += advance_and_return();  // '.'

                    // Read fractional part
                    while (!at_end() && is_digit(current())) {
                        number_str += advance_and_return();
                    }
                }
            }

            // Check for exponent (for both hex and decimal)
            if ((is_hex && (current() == 'p' || current() == 'P')) ||
                (!is_hex && (current() == 'e' || current() == 'E'))) {
                is_float = true;
                number_str += advance_and_return();  // 'e', 'E', 'p', or 'P'

                // Optional sign
                if (current() == '+' || current() == '-') {
                    number_str += advance_and_return();
                }

                // Exponent digits
                if (!is_digit(current())) {
                    report_error("malformed number exponent");
                    return {TokenType::Invalid, number_str, start_location};
                }

                while (!at_end() && is_digit(current())) {
                    number_str += advance_and_return();
                }
            }

            // Parse the number value
            if (is_float) {
                try {
                    Number value = std::stod(number_str);
                    return {TokenType::Number, number_str, start_location, value};
                } catch (const std::exception&) {
                    report_error("invalid number format");
                    return {TokenType::Invalid, number_str, start_location};
                }
            } else {
                try {
                    Int value =
                        is_hex ? std::stoll(number_str, nullptr, 16) : std::stoll(number_str);
                    return {TokenType::Number, number_str, start_location, value};
                } catch (const std::exception&) {
                    report_error("invalid integer format");
                    return {TokenType::Invalid, number_str, start_location};
                }
            }
        }

        Token read_string(const SourceLocation& start_location) {
            const char quote = advance_and_return();  // Skip opening quote
            String result;

            while (!at_end() && current() != quote) {
                if (is_newline(current())) {
                    report_error("unfinished string");
                    return {TokenType::Invalid, result, start_location};
                }

                if (current() == '\\') {
                    advance();  // Skip backslash
                    if (at_end()) {
                        report_error("unfinished string");
                        return {TokenType::Invalid, result, start_location};
                    }

                    char escaped = advance_and_return();
                    switch (escaped) {
                        case 'a':
                            result += '\a';
                            break;
                        case 'b':
                            result += '\b';
                            break;
                        case 'f':
                            result += '\f';
                            break;
                        case 'n':
                            result += '\n';
                            break;
                        case 'r':
                            result += '\r';
                            break;
                        case 't':
                            result += '\t';
                            break;
                        case 'v':
                            result += '\v';
                            break;
                        case '\\':
                            result += '\\';
                            break;
                        case '"':
                            result += '"';
                            break;
                        case '\'':
                            result += '\'';
                            break;
                        case '\n':
                            result += '\n';
                            break;
                        case '\r':
                            result += '\n';
                            if (current() == '\n')
                                advance();  // Handle \r\n
                            break;
                        case 'z':  // Skip whitespace
                            while (!at_end() && (is_space(current()) || is_newline(current()))) {
                                advance();
                            }
                            break;
                        case 'x':  // Hexadecimal escape
                            if (at_end() || !is_hex_digit(current())) {
                                report_error("invalid hexadecimal escape sequence");
                                return {TokenType::Invalid, result, start_location};
                            }
                            {
                                int hex1 = hex_value(advance_and_return());
                                if (at_end() || !is_hex_digit(current())) {
                                    report_error("invalid hexadecimal escape sequence");
                                    return {TokenType::Invalid, result, start_location};
                                }
                                int hex2 = hex_value(advance_and_return());
                                result += static_cast<char>(hex1 * 16 + hex2);
                            }
                            break;
                        default:
                            if (is_digit(escaped)) {
                                // Decimal escape sequence
                                int value = escaped - '0';
                                for (int i = 0; i < 2 && !at_end() && is_digit(current()); ++i) {
                                    value = value * 10 + (advance_and_return() - '0');
                                }
                                if (value > 255) {
                                    report_error("decimal escape sequence out of range");
                                    return {TokenType::Invalid, result, start_location};
                                }
                                result += static_cast<char>(value);
                            } else {
                                report_error("invalid escape sequence");
                                return {TokenType::Invalid, result, start_location};
                            }
                            break;
                    }
                } else {
                    result += advance_and_return();
                }
            }

            if (at_end()) {
                report_error("unfinished string");
                return {TokenType::Invalid, result, start_location};
            }

            advance();  // Skip closing quote
            return {TokenType::String, result, start_location};
        }

        int check_long_string_separator() {
            if (current() != '[')
                return -1;

            Size saved_pos = position_;
            Size saved_line = line_;
            Size saved_col = column_;

            advance();  // Skip '['
            int level = 0;

            while (current() == '=') {
                ++level;
                advance();
            }

            if (current() == '[') {
                advance();  // Skip second '['
                return level;
            }

            // Restore position if not a long string
            position_ = saved_pos;
            line_ = saved_line;
            column_ = saved_col;
            return -1;
        }

        Token read_long_string(const SourceLocation& start_location, int sep_level) {
            String result;

            // The opening sequence has already been consumed by check_long_string_separator
            // Skip first newline if present
            if (is_newline(current())) {
                advance();
                if (current() == '\n' && source_[position_ - 1] == '\r') {
                    advance();
                }
            }

            while (!at_end()) {
                if (current() == ']') {
                    // Check for closing sequence
                    Size saved_pos = position_;
                    Size saved_line = line_;
                    Size saved_col = column_;

                    advance();  // Skip ']'
                    int close_level = 0;

                    // Count the number of '=' characters
                    while (!at_end() && current() == '=') {
                        ++close_level;
                        advance();
                    }

                    // Check if we have the exact number of '=' and a final ']'
                    if (close_level == sep_level && !at_end() && current() == ']') {
                        advance();  // Skip final ']'
                        return {TokenType::String, result, start_location};
                    }

                    // Not the closing sequence, restore and add to result
                    position_ = saved_pos;
                    line_ = saved_line;
                    column_ = saved_col;
                    result += advance_and_return();
                } else {
                    result += advance_and_return();
                }
            }

            report_error("unfinished long string");
            return {TokenType::Invalid, result, start_location};
        }

        void skip_long_comment(int sep_level) {
            // The opening sequence has already been consumed by check_long_string_separator
            while (!at_end()) {
                if (current() == ']') {
                    // Check for closing sequence
                    Size saved_pos = position_;
                    Size saved_line = line_;
                    Size saved_col = column_;

                    advance();  // Skip ']'
                    int close_level = 0;

                    // Count the number of '=' characters
                    while (!at_end() && current() == '=') {
                        ++close_level;
                        advance();
                    }

                    // Check if we have the exact number of '=' and a final ']'
                    if (close_level == sep_level && !at_end() && current() == ']') {
                        advance();  // Skip final ']'
                        return;
                    }

                    // Not the closing sequence, restore
                    position_ = saved_pos;
                    line_ = saved_line;
                    column_ = saved_col;
                    advance();
                } else {
                    advance();
                }
            }

            report_error("unfinished long comment");
        }

        Token read_operator_or_delimiter(const SourceLocation& start_location) {
            const char c = advance_and_return();

            switch (c) {
                case '+':
                    return {TokenType::Plus, "+", start_location};
                case '-':
                    return {TokenType::Minus, "-", start_location};
                case '*':
                    return {TokenType::Multiply, "*", start_location};
                case '/':
                    if (current() == '/') {
                        advance();
                        return {TokenType::IntegerDivide, "//", start_location};
                    }
                    return {TokenType::Divide, "/", start_location};
                case '%':
                    return {TokenType::Modulo, "%", start_location};
                case '^':
                    return {TokenType::Power, "^", start_location};
                case '#':
                    return {TokenType::Length, "#", start_location};
                case '&':
                    return {TokenType::BitwiseAnd, "&", start_location};
                case '|':
                    return {TokenType::BitwiseOr, "|", start_location};
                case '~':
                    if (current() == '=') {
                        advance();
                        return {TokenType::NotEqual, "~=", start_location};
                    }
                    return {TokenType::BitwiseNot, "~", start_location};
                case '<':
                    if (current() == '=') {
                        advance();
                        return {TokenType::LessEqual, "<=", start_location};
                    } else if (current() == '<') {
                        advance();
                        return {TokenType::ShiftLeft, "<<", start_location};
                    }
                    return {TokenType::Less, "<", start_location};
                case '>':
                    if (current() == '=') {
                        advance();
                        return {TokenType::GreaterEqual, ">=", start_location};
                    } else if (current() == '>') {
                        advance();
                        return {TokenType::ShiftRight, ">>", start_location};
                    }
                    return {TokenType::Greater, ">", start_location};
                case '=':
                    if (current() == '=') {
                        advance();
                        return {TokenType::Equal, "==", start_location};
                    }
                    return {TokenType::Assign, "=", start_location};
                case '.':
                    if (current() == '.') {
                        advance();
                        if (current() == '.') {
                            advance();
                            return {TokenType::Ellipsis, "...", start_location};
                        }
                        return {TokenType::Concat, "..", start_location};
                    }
                    return {TokenType::Dot, ".", start_location};
                case ':':
                    if (current() == ':') {
                        advance();
                        return {TokenType::DoubleColon, "::", start_location};
                    }
                    return {TokenType::Colon, ":", start_location};
                case '(':
                    return {TokenType::LeftParen, "(", start_location};
                case ')':
                    return {TokenType::RightParen, ")", start_location};
                case '{':
                    return {TokenType::LeftBrace, "{", start_location};
                case '}':
                    return {TokenType::RightBrace, "}", start_location};
                case '[':
                    return {TokenType::LeftBracket, "[", start_location};
                case ']':
                    return {TokenType::RightBracket, "]", start_location};
                case ';':
                    return {TokenType::Semicolon, ";", start_location};
                case ',':
                    return {TokenType::Comma, ",", start_location};
                default:
                    report_error("unexpected character: '" + String(1, c) + "'");
                    return {TokenType::Invalid, String(1, c), start_location};
            }
        }
    };

    Lexer::Lexer(StringView source, String filename)
        : impl_(std::make_unique<Impl>(source, std::move(filename))) {}

    Lexer::Lexer(std::istream& input, String filename) : impl_(nullptr) {
        // Read entire stream into string
        std::ostringstream buffer;
        buffer << input.rdbuf();
        String source = buffer.str();
        impl_ = std::make_unique<Impl>(source, std::move(filename));
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
        Token token;
        do {
            token = next_token();
            tokens.push_back(token);
        } while (token.type != TokenType::EndOfFile);
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
            case TokenType::IntegerDivide:
                return "IntegerDivide";
            case TokenType::Modulo:
                return "Modulo";
            case TokenType::Power:
                return "Power";
            case TokenType::Length:
                return "Length";
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
            case TokenType::BitwiseAnd:
                return "BitwiseAnd";
            case TokenType::BitwiseOr:
                return "BitwiseOr";
            case TokenType::BitwiseXor:
                return "BitwiseXor";
            case TokenType::BitwiseNot:
                return "BitwiseNot";
            case TokenType::ShiftLeft:
                return "ShiftLeft";
            case TokenType::ShiftRight:
                return "ShiftRight";
            case TokenType::LeftParen:
                return "LeftParen";
            case TokenType::RightParen:
                return "RightParen";
            case TokenType::LeftBrace:
                return "LeftBrace";
            case TokenType::RightBrace:
                return "RightBrace";
            case TokenType::LeftBracket:
                return "LeftBracket";
            case TokenType::RightBracket:
                return "RightBracket";
            case TokenType::Semicolon:
                return "Semicolon";
            case TokenType::Comma:
                return "Comma";
            case TokenType::Dot:
                return "Dot";
            case TokenType::Colon:
                return "Colon";
            case TokenType::DoubleColon:
                return "DoubleColon";
            case TokenType::Ellipsis:
                return "Ellipsis";
            case TokenType::EndOfFile:
                return "EndOfFile";
            case TokenType::Newline:
                return "Newline";
            case TokenType::Comment:
                return "Comment";
            case TokenType::Invalid:
                return "Invalid";
            default:
                return "Unknown";
        }
    }

    Optional<TokenType> string_to_keyword(StringView str) noexcept {
        static const std::unordered_map<StringView, TokenType> keywords = {
            {"and", TokenType::And},
            {"break", TokenType::Break},
            {"do", TokenType::Do},
            {"else", TokenType::Else},
            {"elseif", TokenType::Elseif},
            {"end", TokenType::End},
            {"false", TokenType::False},
            {"for", TokenType::For},
            {"function", TokenType::Function},
            {"goto", TokenType::Goto},
            {"if", TokenType::If},
            {"in", TokenType::In},
            {"local", TokenType::Local},
            {"nil", TokenType::Nil},
            {"not", TokenType::Not},
            {"or", TokenType::Or},
            {"repeat", TokenType::Repeat},
            {"return", TokenType::Return},
            {"then", TokenType::Then},
            {"true", TokenType::True},
            {"until", TokenType::Until},
            {"while", TokenType::While}};

        auto it = keywords.find(str);
        return it != keywords.end() ? Optional<TokenType>{it->second} : std::nullopt;
    }

}  // namespace rangelua::frontend
