/**
 * @file test_main.cpp
 * @brief Custom Catch2 main with enhanced logging support for RangeLua tests
 * @version 0.1.0
 */

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include <rangelua/utils/logger.hpp>

#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Parse logging-related command line arguments
 */
void parse_logging_args(int argc, char* argv[]) {
    std::vector<std::string> module_log_configs;
    std::string global_log_level = "off"; // Default to off for tests
    std::string log_file;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--test-log-level" && i + 1 < argc) {
            global_log_level = argv[++i];
        } else if (arg == "--test-module-log" && i + 1 < argc) {
            module_log_configs.emplace_back(argv[++i]);
        } else if (arg == "--test-log-file" && i + 1 < argc) {
            log_file = argv[++i];
        } else if (arg == "--test-log-help") {
            std::cout << "\nRangeLua Test Logging Options:\n";
            std::cout << "  --test-log-level LEVEL      Set global log level (trace, debug, info, warn, error, off)\n";
            std::cout << "  --test-module-log MOD:LVL   Set module-specific log level (e.g., parser:debug)\n";
            std::cout << "  --test-log-file FILE        Write logs to file\n";
            std::cout << "  --test-log-help             Show this help message\n\n";
            std::cout << "Available modules: lexer, parser, codegen, optimizer, vm, memory, gc\n\n";
            std::cout << "Examples:\n";
            std::cout << "  xmake run rangelua_test --test-log-level debug\n";
            std::cout << "  xmake run rangelua_test --test-module-log parser:trace --test-module-log lexer:debug\n";
            std::cout << "  xmake run rangelua_test \"[parser]\" --test-log-level info\n\n";
        }
    }

    // Initialize logging system
    auto log_level = rangelua::utils::Logger::string_to_log_level(global_log_level);
    rangelua::utils::Logger::initialize("rangelua_test", log_level);

    // Configure module-specific log levels
    if (!module_log_configs.empty()) {
        rangelua::utils::Logger::configure_from_args(module_log_configs);
    }

    // Add file sink if specified
    if (!log_file.empty()) {
        rangelua::utils::Logger::add_file_sink(log_file);
    }

    // Set a test-friendly pattern
    rangelua::utils::Logger::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
}

/**
 * @brief Custom main function for Catch2 with logging support
 */
int main(int argc, char* argv[]) {
    // Parse and configure logging before running tests
    parse_logging_args(argc, argv);

    // Create Catch2 session
    Catch::Session session;

    // Filter out our custom logging arguments before passing to Catch2
    std::vector<char*> catch_args;
    catch_args.push_back(argv[0]); // Program name

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Skip our custom logging arguments
        if (arg == "--test-log-level" || arg == "--test-module-log" || arg == "--test-log-file") {
            ++i; // Skip the next argument too (the value)
        } else if (arg == "--test-log-help") {
            // Skip this argument
        } else {
            catch_args.push_back(argv[i]);
        }
    }

    // Parse Catch2 arguments
    int returnCode = session.applyCommandLine(static_cast<int>(catch_args.size()), catch_args.data());
    if (returnCode != 0) {
        return returnCode;
    }

    // Run the tests
    int result = session.run();

    // Shutdown logging system
    rangelua::utils::Logger::shutdown();

    return result;
}
