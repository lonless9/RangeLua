/**
 * @file main.cpp
 * @brief RangeLua interpreter main entry point
 * @version 0.1.0
 */

#include <rangelua/rangelua.hpp>
#include <rangelua/utils/logger.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace rangelua;

/**
 * @brief Command line options
 */
struct Options {
    std::vector<std::string> files;
    std::string log_level = "off";
    std::vector<std::string> module_log_levels;
    std::string log_file;
    bool interactive = false;
    bool version = false;
    bool help = false;
    bool debug = false;
};

/**
 * @brief Parse command line arguments
 */
Options parse_args(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            opts.help = true;
        } else if (arg == "--version" || arg == "-v") {
            opts.version = true;
        } else if (arg == "--interactive" || arg == "-i") {
            opts.interactive = true;
        } else if (arg == "--debug" || arg == "-d") {
            opts.debug = true;
        } else if (arg == "--log-level") {
            if (i + 1 < argc) {
                opts.log_level = argv[++i];
            }
        } else if (arg == "--module-log") {
            if (i + 1 < argc) {
                opts.module_log_levels.push_back(argv[++i]);
            }
        } else if (arg == "--log-file") {
            if (i + 1 < argc) {
                opts.log_file = argv[++i];
            }
        } else if (!arg.empty() && arg[0] != '-') {
            opts.files.push_back(arg);
        }
    }

    return opts;
}

/**
 * @brief Print help message
 */
void print_help() {
    std::cout << "RangeLua " << rangelua::version() << " - Modern C++20 Lua Interpreter\n\n";
    std::cout << "Usage: rangelua [options] [script [args]]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help          Show this help message\n";
    std::cout << "  -v, --version       Show version information\n";
    std::cout << "  -i, --interactive   Enter interactive mode\n";
    std::cout << "  -d, --debug         Enable debug mode\n";
    std::cout
        << "  --log-level LEVEL   Set global log level (trace, debug, info, warn, error, off)\n";
    std::cout << "                      When specified without --module-log, enables all modules\n";
    std::cout << "  --module-log MOD:LVL Set module-specific log level (e.g., parser:debug)\n";
    std::cout
        << "                      When specified, only enables explicitly mentioned modules\n";
    std::cout << "  --log-file FILE     Write logs to file\n\n";
    std::cout << "Available modules: lexer, parser, codegen, optimizer, vm, memory, gc\n\n";
    std::cout << "Logging behavior:\n";
    std::cout << "  - Explicit modules: --module-log \"parser:debug\" (only parser logs)\n";
    std::cout << "  - All modules: --log-level debug (all modules at debug level)\n";
    std::cout << "  - Clean output: --log-level off or no logging args\n\n";
    std::cout << "Examples:\n";
    std::cout << "  rangelua script.lua                    # Execute script (no logs)\n";
    std::cout << "  rangelua --log-level debug script.lua  # All modules debug logging\n";
    std::cout << "  rangelua --module-log \"parser:debug\" script.lua  # Only parser debug\n";
    std::cout << "  rangelua -i                            # Interactive mode\n";
}

/**
 * @brief Print version information
 */
void print_version() {
    std::cout << "RangeLua " << rangelua::version() << "\n";
    std::cout << "Compatible with Lua " << rangelua::lua_version() << "\n";
    std::cout << "Built with C++20 features\n";
    std::cout << "Copyright (c) 2024 RangeLua Project\n";
}

/**
 * @brief Interactive REPL
 */
void run_interactive() {
    std::cout << "RangeLua " << rangelua::version() << " Interactive Mode\n";
    std::cout << "Type 'exit' to quit\n\n";

    api::State state;
    std::string line;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        auto result = state.execute(line, "<interactive>");
        if (std::holds_alternative<std::vector<runtime::Value>>(result)) {
            // Print results
            const auto& values = std::get<std::vector<runtime::Value>>(result);
            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0)
                    std::cout << "\t";
                std::cout << values[i].debug_string();
            }
            if (!values.empty()) {
                std::cout << "\n";
            }
        } else {
            std::cerr << "Error: " << static_cast<int>(std::get<ErrorCode>(result)) << "\n";
        }
    }
}

/**
 * @brief Execute file
 */
int execute_file(const std::string& filename) {
    api::State state;

    auto result = state.execute_file(filename);
    if (std::holds_alternative<std::vector<runtime::Value>>(result)) {
        return 0;
    } else {
        std::cerr << "Error executing file '" << filename
                  << "': " << static_cast<int>(std::get<ErrorCode>(result)) << "\n";
        return 1;
    }
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    auto opts = parse_args(argc, argv);

    if (opts.help) {
        print_help();
        return 0;
    }

    if (opts.version) {
        print_version();
        return 0;
    }

    // Initialize logging with new activation strategy
    auto log_level = utils::Logger::string_to_log_level(opts.log_level);
    utils::Logger::initialize("rangelua", log_level);

    // Apply new logging activation strategy
    if (!opts.module_log_levels.empty()) {
        // Explicit module logging: activate only specified modules
        utils::Logger::configure_from_args(opts.module_log_levels);
    } else if (log_level != utils::Logger::LogLevel::off) {
        // Default all-module logging: when log level is specified but no modules are explicit
        utils::Logger::enable_all_modules(log_level);
    }
    // If log_level is "off" or no logging args provided, keep all modules disabled (default
    // behavior)

    if (!opts.log_file.empty()) {
        utils::Logger::add_file_sink(opts.log_file);
    }

    // Initialize RangeLua
    auto init_result = rangelua::initialize();
    if (is_error(init_result)) {
        std::cerr << "Failed to initialize RangeLua\n";
        return 1;
    }

    int exit_code = 0;

    try {
        if (opts.files.empty() || opts.interactive) {
            run_interactive();
        } else {
            exit_code = execute_file(opts.files[0]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        exit_code = 1;
    }

    // Cleanup
    rangelua::cleanup();
    utils::Logger::shutdown();

    return exit_code;
}
