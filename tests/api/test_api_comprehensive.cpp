/**
 * @file test_api_comprehensive.cpp
 * @brief Comprehensive tests for the RangeLua API module
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <rangelua/api/api.hpp>

TEST_CASE("API Lifecycle", "[api][lifecycle]") {
    SECTION("Initialize and cleanup") {
        // Test multiple initializations
        rangelua::api::initialize();
        rangelua::api::initialize();  // Should not crash

        // Test version info
        const auto& ver = rangelua::api::version();
        REQUIRE(ver.MAJOR == 0);
        REQUIRE(ver.MINOR == 1);
        REQUIRE(ver.PATCH == 0);
        REQUIRE(std::string(ver.STRING) == "0.1.0");
        REQUIRE(std::string(ver.NAME) == "RangeLua");

        // Test cleanup
        rangelua::api::cleanup();
        rangelua::api::cleanup();  // Should not crash
    }
}

TEST_CASE("Basic API functionality", "[api][basic]") {
    rangelua::api::initialize();

    SECTION("Table creation") {
        auto table = rangelua::api::Table();
        REQUIRE(table.is_valid());
        REQUIRE(table.is_table());
        REQUIRE(table.empty());
    }

    SECTION("Function creation") {
        auto func = rangelua::api::function_factory::from_c_function(
            [](const std::vector<rangelua::runtime::Value>&) -> std::vector<rangelua::runtime::Value> {
                return {rangelua::runtime::Value("test")};
            }
        );
        REQUIRE(func.is_valid());
        REQUIRE(func.is_c_function());
    }

    SECTION("Coroutine creation") {
        auto coro = rangelua::api::Coroutine();
        REQUIRE(coro.is_valid());
        REQUIRE(coro.is_suspended());
    }

    rangelua::api::cleanup();
}
