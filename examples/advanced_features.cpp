/**
 * @file advanced_features.cpp
 * @brief Advanced features example for RangeLua
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
        
        // Example with tables and functions
        auto result = state.execute(R"(
            local function fibonacci(n)
                if n <= 1 then
                    return n
                else
                    return fibonacci(n-1) + fibonacci(n-2)
                end
            end
            
            local results = {}
            for i = 1, 10 do
                results[i] = fibonacci(i)
            end
            
            return results
        )", "advanced_example");
        
        if (result) {
            std::cout << "Fibonacci results: ";
            for (const auto& value : result.value()) {
                std::cout << value.debug_string() << " ";
            }
            std::cout << "\n";
        } else {
            std::cerr << "Execution failed with error: " 
                      << static_cast<int>(result.error()) << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    // Cleanup
    rangelua::cleanup();
    return 0;
}
