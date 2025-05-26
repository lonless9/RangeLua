/**
 * @file performance_demo.cpp
 * @brief Performance demonstration for RangeLua
 * @version 0.1.0
 */

#include <rangelua/rangelua.hpp>
#include <rangelua/utils/profiler.hpp>

#include <chrono>
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

        // Performance test: compute primes
        const char* prime_code = R"(
            local function is_prime(n)
                if n < 2 then return false end
                if n == 2 then return true end
                if n % 2 == 0 then return false end

                for i = 3, math.sqrt(n), 2 do
                    if n % i == 0 then return false end
                end
                return true
            end

            local primes = {}
            local count = 0
            for i = 2, 1000 do
                if is_prime(i) then
                    count = count + 1
                    primes[count] = i
                end
            end

            return count
        )";

        // Measure execution time
        rangelua::utils::Profiler::start("prime_calculation");
        auto start = std::chrono::high_resolution_clock::now();

        auto result = state.execute(prime_code, "performance_test");

        auto end = std::chrono::high_resolution_clock::now();
        rangelua::utils::Profiler::end("prime_calculation");

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (result) {
            std::cout << "Found primes: ";
            for (const auto& value : result.value()) {
                std::cout << value.debug_string() << " ";
            }
            std::cout << "\n";
            std::cout << "Execution time: " << duration.count() << " microseconds\n";
        } else {
            std::cerr << "Execution failed with error: " << static_cast<int>(result.error())
                      << "\n";
        }

        // Print profiling results
        auto profiling_results = rangelua::utils::Profiler::results();
        for (const auto& [name, time] : profiling_results) {
            std::cout << "Profile [" << name << "]: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(time).count()
                      << " microseconds\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    // Cleanup
    rangelua::cleanup();
    return 0;
}
