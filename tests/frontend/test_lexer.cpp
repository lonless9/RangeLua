/**
 * @file test_lexer.cpp
 * @brief Comprehensive tests for the RangeLua lexer
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <rangelua/frontend/lexer.hpp>

using namespace rangelua;
using frontend::TokenType;

TEST_CASE("Lexer - Basic Token Types", "[lexer][basic]") {
    SECTION("Keywords") {
        frontend::Lexer lexer("and break do else elseif end false for function goto if in local nil not or repeat return then true until while", "<test>");

        std::vector<TokenType> expected = {
            TokenType::And, TokenType::Break, TokenType::Do, TokenType::Else, TokenType::Elseif,
            TokenType::End, TokenType::False, TokenType::For, TokenType::Function, TokenType::Goto,
            TokenType::If, TokenType::In, TokenType::Local, TokenType::Nil, TokenType::Not,
            TokenType::Or, TokenType::Repeat, TokenType::Return, TokenType::Then, TokenType::True,
            TokenType::Until, TokenType::While, TokenType::EndOfFile
        };

        for (auto expected_type : expected) {
            auto token = lexer.next_token();
            REQUIRE(token.type == expected_type);
        }
    }

    SECTION("Identifiers") {
        frontend::Lexer lexer("variable _private __internal a1 test123", "<test>");

        for (int i = 0; i < 5; ++i) {
            auto token = lexer.next_token();
            REQUIRE(token.type == TokenType::Identifier);
        }

        auto eof = lexer.next_token();
        REQUIRE(eof.type == TokenType::EndOfFile);
    }

    SECTION("Numbers - Integers") {
        frontend::Lexer lexer("123 0 999", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Number);
        REQUIRE(token1.integer_value.has_value());
        REQUIRE(token1.integer_value.value() == 123);

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Number);
        REQUIRE(token2.integer_value.has_value());
        REQUIRE(token2.integer_value.value() == 0);

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Number);
        REQUIRE(token3.integer_value.has_value());
        REQUIRE(token3.integer_value.value() == 999);
    }

    SECTION("Numbers - Floats") {
        frontend::Lexer lexer("3.14 0.5 123.456", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Number);
        REQUIRE(token1.number_value.has_value());
        REQUIRE(token1.number_value.value() == 3.14);

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Number);
        REQUIRE(token2.number_value.has_value());
        REQUIRE(token2.number_value.value() == 0.5);

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Number);
        REQUIRE(token3.number_value.has_value());
        REQUIRE(token3.number_value.value() == 123.456);
    }

    SECTION("Numbers - Hexadecimal") {
        frontend::Lexer lexer("0x10 0xFF 0xabc", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Number);
        REQUIRE(token1.integer_value.has_value());
        REQUIRE(token1.integer_value.value() == 16);

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Number);
        REQUIRE(token2.integer_value.has_value());
        REQUIRE(token2.integer_value.value() == 255);

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Number);
        REQUIRE(token3.integer_value.has_value());
        REQUIRE(token3.integer_value.value() == 0xabc);
    }
}

TEST_CASE("Lexer - String Literals", "[lexer][strings]") {
    SECTION("Simple Strings") {
        frontend::Lexer lexer(R"("hello" 'world')", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::String);
        REQUIRE(token1.value == "hello");

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::String);
        REQUIRE(token2.value == "world");
    }

    SECTION("Escape Sequences") {
        frontend::Lexer lexer(R"("hello\nworld" "tab\there" "quote\"test")", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::String);
        REQUIRE(token1.value == "hello\nworld");

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::String);
        REQUIRE(token2.value == "tab\there");

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::String);
        REQUIRE(token3.value == "quote\"test");
    }

    SECTION("Hexadecimal Escapes") {
        frontend::Lexer lexer(R"("\x41\x42\x43")", "<test>");

        auto token = lexer.next_token();
        REQUIRE(token.type == TokenType::String);
        REQUIRE(token.value == "ABC");
    }

    SECTION("Long Strings") {
        frontend::Lexer lexer("[[hello world]] [===[nested [brackets]=] here]===]", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::String);
        REQUIRE(token1.value == "hello world");

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::String);
        REQUIRE(token2.value == "nested [brackets]=] here");
    }
}

TEST_CASE("Lexer - Operators and Delimiters", "[lexer][operators]") {
    SECTION("Arithmetic Operators") {
        frontend::Lexer lexer("+ - * / % ^", "<test>");

        std::vector<TokenType> expected = {
            TokenType::Plus, TokenType::Minus, TokenType::Multiply,
            TokenType::Divide, TokenType::Modulo, TokenType::Power
        };

        for (auto expected_type : expected) {
            auto token = lexer.next_token();
            REQUIRE(token.type == expected_type);
        }
    }

    SECTION("Comparison Operators") {
        frontend::Lexer lexer("== ~= < <= > >=", "<test>");

        std::vector<TokenType> expected = {
            TokenType::Equal, TokenType::NotEqual, TokenType::Less,
            TokenType::LessEqual, TokenType::Greater, TokenType::GreaterEqual
        };

        for (auto expected_type : expected) {
            auto token = lexer.next_token();
            REQUIRE(token.type == expected_type);
        }
    }

    SECTION("Bitwise Operators") {
        frontend::Lexer lexer("& | ~ << >>", "<test>");

        std::vector<TokenType> expected = {
            TokenType::BitwiseAnd, TokenType::BitwiseOr, TokenType::BitwiseXor,
            TokenType::ShiftLeft, TokenType::ShiftRight
        };

        for (auto expected_type : expected) {
            auto token = lexer.next_token();
            REQUIRE(token.type == expected_type);
        }
    }

    SECTION("Delimiters") {
        frontend::Lexer lexer("( ) { } [ ] ; , . : :: ...", "<test>");

        std::vector<TokenType> expected = {
            TokenType::LeftParen, TokenType::RightParen, TokenType::LeftBrace, TokenType::RightBrace,
            TokenType::LeftBracket, TokenType::RightBracket, TokenType::Semicolon, TokenType::Comma,
            TokenType::Dot, TokenType::Colon, TokenType::DoubleColon, TokenType::Ellipsis
        };

        for (auto expected_type : expected) {
            auto token = lexer.next_token();
            REQUIRE(token.type == expected_type);
        }
    }

    SECTION("Concatenation") {
        frontend::Lexer lexer("..", "<test>");

        auto token = lexer.next_token();
        REQUIRE(token.type == TokenType::Concat);
        REQUIRE(token.value == "..");
    }
}

TEST_CASE("Lexer - Comments", "[lexer][comments]") {
    SECTION("Single Line Comments") {
        frontend::Lexer lexer("local x -- this is a comment\nlocal y", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Local);

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Identifier);
        REQUIRE(token2.value == "x");

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Local);

        auto token4 = lexer.next_token();
        REQUIRE(token4.type == TokenType::Identifier);
        REQUIRE(token4.value == "y");
    }

    SECTION("Multi-line Comments") {
        frontend::Lexer lexer("local x --[[this is a\nmulti-line comment]] local y", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Local);

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Identifier);
        REQUIRE(token2.value == "x");

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Local);

        auto token4 = lexer.next_token();
        REQUIRE(token4.type == TokenType::Identifier);
        REQUIRE(token4.value == "y");
    }
}

TEST_CASE("Lexer - Error Handling", "[lexer][errors]") {
    SECTION("Invalid Characters") {
        frontend::Lexer lexer("@", "<test>");

        auto token = lexer.next_token();
        REQUIRE(token.type == TokenType::Invalid);
        REQUIRE(lexer.has_errors());
    }

    SECTION("Unfinished String") {
        frontend::Lexer lexer("\"unfinished", "<test>");

        auto token = lexer.next_token();
        REQUIRE(token.type == TokenType::Invalid);
        REQUIRE(lexer.has_errors());
    }

    SECTION("Invalid Escape Sequence") {
        frontend::Lexer lexer("\"\\q\"", "<test>");

        auto token = lexer.next_token();
        REQUIRE(token.type == TokenType::Invalid);
        REQUIRE(lexer.has_errors());
    }
}

TEST_CASE("Lexer - Complex Scenarios", "[lexer][complex]") {
    SECTION("Mixed Token Types") {
        frontend::Lexer lexer("function test(x, y) return x + y end", "<test>");

        std::vector<TokenType> expected = {
            TokenType::Function, TokenType::Identifier, TokenType::LeftParen,
            TokenType::Identifier, TokenType::Comma, TokenType::Identifier,
            TokenType::RightParen, TokenType::Return, TokenType::Identifier,
            TokenType::Plus, TokenType::Identifier, TokenType::End, TokenType::EndOfFile
        };

        for (auto expected_type : expected) {
            auto token = lexer.next_token();
            REQUIRE(token.type == expected_type);
        }
    }

    SECTION("Whitespace Handling") {
        frontend::Lexer lexer("  \t\n  local   \t x  \n  =  \t 42  \n  ", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Local);

        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Identifier);
        REQUIRE(token2.value == "x");

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Assign);

        auto token4 = lexer.next_token();
        REQUIRE(token4.type == TokenType::Number);
        REQUIRE(token4.integer_value.value() == 42);
    }

    SECTION("Number Edge Cases") {
        frontend::Lexer lexer("0x 0.5.5 1e 1e+ 1e-", "<test>");

        // 0x should be invalid
        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Invalid);

        // 0.5 followed by .5 (should be two tokens)
        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Number);
        REQUIRE(token2.number_value.value() == 0.5);

        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::Number);
        REQUIRE(token3.number_value.value() == 0.5);

        // Invalid exponents
        auto token4 = lexer.next_token();
        REQUIRE(token4.type == TokenType::Invalid);

        auto token5 = lexer.next_token();
        REQUIRE(token5.type == TokenType::Invalid);

        auto token6 = lexer.next_token();
        REQUIRE(token6.type == TokenType::Invalid);
    }

    SECTION("String Edge Cases") {
        frontend::Lexer lexer(R"("\z   \n" "\x41\x42" "\65\66\67")", "<test>");

        // \z should skip whitespace
        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::String);
        REQUIRE(token1.value == "\n");

        // Hex escapes
        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::String);
        REQUIRE(token2.value == "AB");

        // Decimal escapes
        auto token3 = lexer.next_token();
        REQUIRE(token3.type == TokenType::String);
        REQUIRE(token3.value == "ABC");
    }

    SECTION("Long String Edge Cases") {
        frontend::Lexer lexer("[==[hello [[ world ]] test]==]", "<test>");

        auto token = lexer.next_token();
        REQUIRE(token.type == TokenType::String);
        REQUIRE(token.value == "hello [[ world ]] test");
    }
}

TEST_CASE("Lexer - Location Tracking", "[lexer][location]") {
    SECTION("Line and Column Tracking") {
        frontend::Lexer lexer("local x\n  = 42\n", "<test>");

        auto token1 = lexer.next_token();
        REQUIRE(token1.location.line_ == 1);
        REQUIRE(token1.location.column_ == 1);

        auto token2 = lexer.next_token();
        REQUIRE(token2.location.line_ == 1);
        REQUIRE(token2.location.column_ == 7);

        auto token3 = lexer.next_token();
        REQUIRE(token3.location.line_ == 2);
        REQUIRE(token3.location.column_ == 3);

        auto token4 = lexer.next_token();
        REQUIRE(token4.location.line_ == 2);
        REQUIRE(token4.location.column_ == 5);
    }
}

TEST_CASE("Lexer - Peek Functionality", "[lexer][peek]") {
    SECTION("Peek Without Consuming") {
        frontend::Lexer lexer("local x = 42", "<test>");

        // Peek at first token
        const auto& peeked = lexer.peek_token();
        REQUIRE(peeked.type == TokenType::Local);

        // Next token should be the same
        auto token1 = lexer.next_token();
        REQUIRE(token1.type == TokenType::Local);

        // Peek at second token
        const auto& peeked2 = lexer.peek_token();
        REQUIRE(peeked2.type == TokenType::Identifier);
        REQUIRE(peeked2.value == "x");

        // Next token should be the same
        auto token2 = lexer.next_token();
        REQUIRE(token2.type == TokenType::Identifier);
        REQUIRE(token2.value == "x");
    }
}

TEST_CASE("Lexer - Tokenize All", "[lexer][tokenize]") {
    SECTION("Complete Tokenization") {
        frontend::Lexer lexer("local x = 42", "<test>");

        auto tokens = lexer.tokenize();

        REQUIRE(tokens.size() == 5); // local, x, =, 42, EOF
        REQUIRE(tokens[0].type == TokenType::Local);
        REQUIRE(tokens[1].type == TokenType::Identifier);
        REQUIRE(tokens[2].type == TokenType::Assign);
        REQUIRE(tokens[3].type == TokenType::Number);
        REQUIRE(tokens[4].type == TokenType::EndOfFile);
    }
}
