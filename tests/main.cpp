#include "spdlog/common.h"
#define CATCH_CONFIG_RUNNER
#include "rangelua/utils/logger.hpp"

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

int main(int argc, char* argv[]) {
    rangelua::utils::Logger::initialize("rangelua", spdlog::level::off);
    auto modules = rangelua::utils::Logger::get_available_modules();
    for (const auto& module : modules) {
        rangelua::utils::Logger::set_module_level(module, rangelua::utils::Logger::LogLevel::off);
    }
    int result = Catch::Session().run(argc, argv);

    return result;
}
