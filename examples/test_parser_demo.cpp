#include <rangelua/frontend/parser.hpp>
#include <fstream>
#include <iostream>

int main() {
    // Read the test file
    std::ifstream file("examples/test_parser.lua");
    if (!file) {
        std::cerr << "Could not open test_parser.lua\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    // Create parser
    rangelua::frontend::Parser parser(source, "test_parser.lua");

    // Parse the file
    auto result = parser.parse();

    if (rangelua::is_success(result)) {
        std::cout << "✓ Parsing successful!\n";
        auto program = rangelua::get_value(std::move(result));
        std::cout << "✓ Parsed " << program->statements().size() << " statements\n";
    } else {
        std::cout << "✗ Parsing failed\n";
        auto errors = parser.errors();
        for (const auto& error : errors) {
            std::cout << "Error: " << error.what() << " at "
                      << error.source_location().line_ << ":" << error.source_location().column_ << "\n";
        }
        return 1;
    }

    return 0;
}
