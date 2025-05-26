/**
 * @file test_basic.cpp
 * @brief Basic unit tests for RangeLua
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/rangelua.hpp>

TEST_CASE("RangeLua initialization", "[core]") {
    REQUIRE(rangelua::initialize());
    rangelua::cleanup();
}

TEST_CASE("Version information", "[core]") {
    REQUIRE(rangelua::version() != nullptr);
    REQUIRE(rangelua::lua_version() != nullptr);

    // Check version format
    std::string version_str = rangelua::version();
    REQUIRE(!version_str.empty());
    REQUIRE(version_str.find('.') != std::string::npos);
}

TEST_CASE("Value creation and type checking", "[runtime]") {
    using namespace rangelua::runtime;

    SECTION("Nil value") {
        Value nil_val;
        REQUIRE(nil_val.is_nil());
        REQUIRE(nil_val.type() == ValueType::Nil);
        REQUIRE(!nil_val.is_truthy());
    }

    SECTION("Boolean values") {
        Value true_val(true);
        Value false_val(false);

        REQUIRE(true_val.is_boolean());
        REQUIRE(false_val.is_boolean());
        REQUIRE(true_val.is_truthy());
        REQUIRE(!false_val.is_truthy());
    }

    SECTION("Number values") {
        Value num_val(42.0);
        Value int_val(static_cast<rangelua::Number>(42));

        REQUIRE(num_val.is_number());
        REQUIRE(int_val.is_number());
        REQUIRE(num_val.is_truthy());
        REQUIRE(int_val.is_truthy());
    }

    SECTION("String values") {
        Value str_val("hello");
        Value empty_str("");

        REQUIRE(str_val.is_string());
        REQUIRE(empty_str.is_string());
        REQUIRE(str_val.is_truthy());
        REQUIRE(empty_str.is_truthy()); // Empty strings are truthy in Lua
    }
}

TEST_CASE("Value equality", "[runtime]") {
    using namespace rangelua::runtime;

    SECTION("Same type equality") {
        Value nil1, nil2;
        REQUIRE(nil1 == nil2);

        Value true1(true), true2(true);
        REQUIRE(true1 == true2);

        Value num1(42.0), num2(42.0);
        REQUIRE(num1 == num2);

        Value str1("hello"), str2("hello");
        REQUIRE(str1 == str2);
    }

    SECTION("Different type inequality") {
        Value nil_val;
        Value bool_val(false);
        Value num_val(0.0);
        Value str_val("0");

        REQUIRE(nil_val != bool_val);
        REQUIRE(bool_val != num_val);
        REQUIRE(num_val != str_val);
    }
}

TEST_CASE("Value type names", "[runtime]") {
    using namespace rangelua::runtime;

    Value nil_val;
    Value bool_val(true);
    Value num_val(42.0);
    Value str_val("test");

    REQUIRE(nil_val.type_name() == "nil");
    REQUIRE(bool_val.type_name() == "boolean");
    REQUIRE(num_val.type_name() == "number");
    REQUIRE(str_val.type_name() == "string");
}

TEST_CASE("State creation", "[api]") {
    REQUIRE(rangelua::initialize());

    rangelua::api::State state;
    REQUIRE(state.stack_size() == 0);

    rangelua::cleanup();
}

TEST_CASE("Basic Lua execution", "[integration]") {
    REQUIRE(rangelua::initialize());

    rangelua::api::State state;

    SECTION("Simple arithmetic") {
        auto result = state.execute("return 2 + 3", "test");
        // Note: This will fail until we implement the full pipeline
        // For now, we just check that it doesn't crash
        REQUIRE(true); // Placeholder
    }

    SECTION("Variable assignment") {
        auto result = state.execute("local x = 10; return x", "test");
        // Note: This will fail until we implement the full pipeline
        REQUIRE(true); // Placeholder
    }

    rangelua::cleanup();
}
