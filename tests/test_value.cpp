/**
 * @file test_value.cpp
 * @brief Comprehensive unit tests for the Value system
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/objects.hpp>

using rangelua::runtime::Value;
using rangelua::runtime::ValueType;
using rangelua::String;
using rangelua::StringView;
using rangelua::Number;
using rangelua::Int;
using rangelua::ErrorCode;

namespace vf = rangelua::runtime::value_factory;

TEST_CASE("Value basic construction and type checking", "[value][basic]") {
    SECTION("Nil value") {
        Value nil_val;
        REQUIRE(nil_val.is_nil());
        REQUIRE(nil_val.type() == ValueType::Nil);
        REQUIRE(nil_val.type_name() == "nil");
        REQUIRE_FALSE(nil_val.is_truthy());
        REQUIRE(nil_val.is_falsy());
    }

    SECTION("Boolean values") {
        Value true_val(true);
        Value false_val(false);

        REQUIRE(true_val.is_boolean());
        REQUIRE(true_val.type() == ValueType::Boolean);
        REQUIRE(true_val.type_name() == "boolean");
        REQUIRE(true_val.is_truthy());

        REQUIRE(false_val.is_boolean());
        REQUIRE_FALSE(false_val.is_truthy());
        REQUIRE(false_val.is_falsy());
    }

    SECTION("Number values") {
        Value num_val(42.5);
        Value int_val(Int(123));

        REQUIRE(num_val.is_number());
        REQUIRE(num_val.type() == ValueType::Number);
        REQUIRE(num_val.type_name() == "number");
        REQUIRE(num_val.is_truthy());

        REQUIRE(int_val.is_number());
        REQUIRE(int_val.is_truthy());
    }

    SECTION("String values") {
        Value str_val("hello");
        Value str_view_val(StringView("world"));
        Value str_move_val(String("test"));

        REQUIRE(str_val.is_string());
        REQUIRE(str_val.type() == ValueType::String);
        REQUIRE(str_val.type_name() == "string");
        REQUIRE(str_val.is_truthy());

        REQUIRE(str_view_val.is_string());
        REQUIRE(str_move_val.is_string());
    }
}

TEST_CASE("Value type conversions", "[value][conversion]") {
    SECTION("Boolean conversion") {
        Value true_val(true);
        Value false_val(false);
        Value nil_val;
        Value num_val(42.0);

        auto true_result = true_val.to_boolean();
        REQUIRE(std::holds_alternative<bool>(true_result));
        REQUIRE(std::get<bool>(true_result) == true);

        auto false_result = false_val.to_boolean();
        REQUIRE(std::holds_alternative<bool>(false_result));
        REQUIRE(std::get<bool>(false_result) == false);

        auto nil_result = nil_val.to_boolean();
        REQUIRE(std::holds_alternative<ErrorCode>(nil_result));

        auto num_result = num_val.to_boolean();
        REQUIRE(std::holds_alternative<ErrorCode>(num_result));
    }

    SECTION("Number conversion") {
        Value num_val(42.5);
        Value str_val("123.45");
        Value invalid_str_val("not a number");
        Value bool_val(true);

        auto num_result = num_val.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(42.5));

        auto str_result = str_val.to_number();
        REQUIRE(std::holds_alternative<Number>(str_result));
        REQUIRE(std::get<Number>(str_result) == Catch::Approx(123.45));

        auto invalid_result = invalid_str_val.to_number();
        REQUIRE(std::holds_alternative<ErrorCode>(invalid_result));

        auto bool_result = bool_val.to_number();
        REQUIRE(std::holds_alternative<ErrorCode>(bool_result));
    }

    SECTION("String conversion") {
        Value str_val("hello");
        Value num_val(42.5);
        Value bool_val(true);

        auto str_result = str_val.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "hello");

        auto num_result = num_val.to_string();
        REQUIRE(std::holds_alternative<String>(num_result));
        // Note: exact string representation may vary
        REQUIRE_FALSE(std::get<String>(num_result).empty());

        auto bool_result = bool_val.to_string();
        REQUIRE(std::holds_alternative<ErrorCode>(bool_result));
    }
}

TEST_CASE("Value arithmetic operations", "[value][arithmetic]") {
    SECTION("Addition") {
        Value a(10.0);
        Value b(5.0);
        Value str_num("3.5");
        Value invalid_str("not a number");

        Value result = a + b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(15.0));

        Value str_result = a + str_num;
        REQUIRE(str_result.is_number());
        auto str_num_result = str_result.to_number();
        REQUIRE(std::holds_alternative<Number>(str_num_result));
        REQUIRE(std::get<Number>(str_num_result) == Catch::Approx(13.5));

        Value invalid_result = a + invalid_str;
        REQUIRE(invalid_result.is_nil());
    }

    SECTION("Subtraction") {
        Value a(10.0);
        Value b(3.0);

        Value result = a - b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(7.0));
    }

    SECTION("Multiplication") {
        Value a(4.0);
        Value b(2.5);

        Value result = a * b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(10.0));
    }

    SECTION("Division") {
        Value a(10.0);
        Value b(2.0);
        Value zero(0.0);

        Value result = a / b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(5.0));

        // Division by zero should result in infinity
        Value inf_result = a / zero;
        REQUIRE(inf_result.is_number());
        auto inf_num_result = inf_result.to_number();
        REQUIRE(std::holds_alternative<Number>(inf_num_result));
        REQUIRE(std::isinf(std::get<Number>(inf_num_result)));
    }

    SECTION("Modulo") {
        Value a(10.0);
        Value b(3.0);

        Value result = a % b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(1.0));
    }

    SECTION("Exponentiation") {
        Value a(2.0);
        Value b(3.0);

        Value result = a ^ b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(8.0));
    }

    SECTION("Unary minus") {
        Value a(5.0);

        Value result = -a;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(-5.0));
    }
}

TEST_CASE("Value bitwise operations", "[value][bitwise]") {
    SECTION("Bitwise AND") {
        Value a(12.0);  // 1100 in binary
        Value b(10.0);  // 1010 in binary

        Value result = a & b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(8.0));  // 1000 in binary
    }

    SECTION("Bitwise OR") {
        Value a(12.0);  // 1100 in binary
        Value b(10.0);  // 1010 in binary

        Value result = a | b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(14.0));  // 1110 in binary
    }

    SECTION("Bitwise NOT") {
        Value a(5.0);

        Value result = ~a;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        // Result depends on the size of UInt, but should be a valid number
        REQUIRE(std::holds_alternative<Number>(num_result));
    }

    SECTION("Left shift") {
        Value a(5.0);   // 101 in binary
        Value b(2.0);   // shift by 2

        Value result = a << b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(20.0));  // 10100 in binary
    }

    SECTION("Right shift") {
        Value a(20.0);  // 10100 in binary
        Value b(2.0);   // shift by 2

        Value result = a >> b;
        REQUIRE(result.is_number());
        auto num_result = result.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(5.0));   // 101 in binary
    }
}

TEST_CASE("Value comparison operations", "[value][comparison]") {
    SECTION("Equality") {
        Value a(42.0);
        Value b(42.0);
        Value c(43.0);
        Value str_a("hello");
        Value str_b("hello");
        Value str_c("world");

        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
        REQUIRE(a != c);

        REQUIRE(str_a == str_b);
        REQUIRE_FALSE(str_a == str_c);
        REQUIRE(str_a != str_c);

        // Different types are not equal
        REQUIRE_FALSE(a == str_a);
    }

    SECTION("Ordering") {
        Value a(10.0);
        Value b(20.0);
        Value c(10.0);

        REQUIRE(a < b);
        REQUIRE_FALSE(b < a);
        REQUIRE(a <= b);
        REQUIRE(a <= c);
        REQUIRE(b > a);
        REQUIRE_FALSE(a > b);
        REQUIRE(b >= a);
        REQUIRE(c >= a);

        // String ordering
        Value str_a("apple");
        Value str_b("banana");

        REQUIRE(str_a < str_b);
        REQUIRE_FALSE(str_b < str_a);
    }

    SECTION("Cross-type comparison") {
        Value num(42.0);
        Value str_num("42");
        Value str_invalid("not a number");

        // Numbers and numeric strings should be comparable
        REQUIRE(num == str_num);  // This might fail if cross-type equality isn't implemented

        // Non-numeric strings should not be comparable to numbers
        REQUIRE_FALSE(num == str_invalid);
    }
}

TEST_CASE("Value string operations", "[value][string]") {
    SECTION("String concatenation") {
        Value str_a("hello");
        Value str_b(" world");
        Value num(42.0);

        Value result = str_a.concat(str_b);
        REQUIRE(result.is_string());
        auto str_result = result.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "hello world");

        // Number to string concatenation
        Value num_concat = str_a.concat(num);
        REQUIRE(num_concat.is_string());
        auto num_str_result = num_concat.to_string();
        REQUIRE(std::holds_alternative<String>(num_str_result));
        // Should contain "hello" and some representation of 42
        REQUIRE(std::get<String>(num_str_result).find("hello") != String::npos);
    }

    SECTION("String length") {
        Value str("hello");
        Value empty_str("");

        Value len = str.length();
        REQUIRE(len.is_number());
        auto len_result = len.to_number();
        REQUIRE(std::holds_alternative<Number>(len_result));
        REQUIRE(std::get<Number>(len_result) == Catch::Approx(5.0));

        Value empty_len = empty_str.length();
        REQUIRE(empty_len.is_number());
        auto empty_len_result = empty_len.to_number();
        REQUIRE(std::holds_alternative<Number>(empty_len_result));
        REQUIRE(std::get<Number>(empty_len_result) == Catch::Approx(0.0));
    }
}

TEST_CASE("Value factory functions", "[value][factory]") {
    SECTION("Basic factories") {
        auto nil_val = vf::nil();
        REQUIRE(nil_val.is_nil());

        auto bool_val = vf::boolean(true);
        REQUIRE(bool_val.is_boolean());
        auto bool_result = bool_val.to_boolean();
        REQUIRE(std::holds_alternative<bool>(bool_result));
        REQUIRE(std::get<bool>(bool_result) == true);

        auto num_val = vf::number(42.5);
        REQUIRE(num_val.is_number());
        auto num_result = num_val.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(42.5));

        auto int_val = vf::integer(123);
        REQUIRE(int_val.is_number());
        auto int_result = int_val.to_number();
        REQUIRE(std::holds_alternative<Number>(int_result));
        REQUIRE(std::get<Number>(int_result) == Catch::Approx(123.0));

        auto str_val = vf::string("test");
        REQUIRE(str_val.is_string());
        auto str_result = str_val.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "test");
    }

    SECTION("Table factory") {
        auto table_val = vf::table();
        REQUIRE(table_val.is_table());

        // Test table with initializer list
        auto init_table = vf::table({
            {vf::string("key1"), vf::number(42.0)},
            {vf::string("key2"), vf::string("value")}
        });
        REQUIRE(init_table.is_table());

        // Test table access
        Value key1 = vf::string("key1");
        Value val1 = init_table.get(key1);
        REQUIRE(val1.is_number());
        auto val1_result = val1.to_number();
        REQUIRE(std::holds_alternative<Number>(val1_result));
        REQUIRE(std::get<Number>(val1_result) == Catch::Approx(42.0));
    }
}

TEST_CASE("Value hash and equality", "[value][hash]") {
    SECTION("Hash consistency") {
        Value a(42.0);
        Value b(42.0);
        Value c(43.0);

        REQUIRE(a.hash() == b.hash());  // Equal values should have equal hashes
        // Different values may or may not have different hashes (hash collisions are allowed)

        Value str_a("hello");
        Value str_b("hello");
        Value str_c("world");

        REQUIRE(str_a.hash() == str_b.hash());
    }

    SECTION("std::hash integration") {
        Value a(42.0);
        Value b(42.0);

        std::hash<Value> hasher;
        REQUIRE(hasher(a) == hasher(b));
    }
}

TEST_CASE("Value debug and utility methods", "[value][utility]") {
    SECTION("Type names") {
        REQUIRE(vf::nil().type_name() == "nil");
        REQUIRE(vf::boolean(true).type_name() == "boolean");
        REQUIRE(vf::number(42.0).type_name() == "number");
        REQUIRE(vf::string("test").type_name() == "string");
        REQUIRE(vf::table().type_name() == "table");
    }

    SECTION("Debug strings") {
        auto nil_debug = vf::nil().debug_string();
        REQUIRE(nil_debug == "nil");

        auto bool_debug = vf::boolean(true).debug_string();
        REQUIRE(bool_debug == "true");

        auto false_debug = vf::boolean(false).debug_string();
        REQUIRE(false_debug == "false");

        auto num_debug = vf::number(42.5).debug_string();
        REQUIRE_FALSE(num_debug.empty());

        auto str_debug = vf::string("hello").debug_string();
        REQUIRE(str_debug == "\"hello\"");
    }
}
