/**
 * @file basic_usage.cpp
 * @brief Basic usage example for RangeLua
 * @version 0.1.0
 */

#include <rangelua/rangelua.hpp>

#include <iostream>

int main() {
    // Initialize RangeLua
    if (!rangelua::initialize()) {
        std::cerr << "Failed to initialize RangeLua\n";
        return 1;
    }

    try {
        // Create a Lua state
        rangelua::api::State state;

        // Execute simple Lua code
        auto result = state.execute(R"(
            local x = 10
            local y = 20
            return x + y
        )",
                                    "basic_example");

        if (result) {
            std::cout << "Result: ";
            for (const auto& value : result.value()) {
                std::cout << value.debug_string() << " ";
            }
            std::cout << "\n";
        } else {
            std::cerr << "Execution failed with error: " << static_cast<int>(result.error())
                      << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    // Cleanup
    rangelua::cleanup();
    return 0;
}
