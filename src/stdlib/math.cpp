/**
 * @file math.cpp
 * @brief Implementation of Lua math library functions for RangeLua
 * @version 0.1.0
 */

#include <rangelua/stdlib/math.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>

namespace rangelua::stdlib::math {

    namespace {
        // Random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1.0);
    }

    std::vector<runtime::Value> abs(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::abs(value))};
    }

    std::vector<runtime::Value> acos(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::acos(value))};
    }

    std::vector<runtime::Value> asin(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::asin(value))};
    }

    std::vector<runtime::Value> atan(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double y = std::get<double>(num_result);

        if (args.size() > 1 && args[1].is_number()) {
            auto x_result = args[1].to_number();
            if (std::holds_alternative<double>(x_result)) {
                double x = std::get<double>(x_result);
                return {runtime::Value(std::atan2(y, x))};
            }
        }

        return {runtime::Value(std::atan(y))};
    }

    std::vector<runtime::Value> ceil(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::ceil(value))};
    }

    std::vector<runtime::Value> cos(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::cos(value))};
    }

    std::vector<runtime::Value> deg(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double radians = std::get<double>(num_result);
        double degrees = radians * 180.0 / M_PI;
        return {runtime::Value(degrees)};
    }

    std::vector<runtime::Value> exp(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::exp(value))};
    }

    std::vector<runtime::Value> floor(runtime::IVMContext* vm,
                                    const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::floor(value))};
    }

    std::vector<runtime::Value> fmod(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.size() < 2 || !args[0].is_number() || !args[1].is_number()) {
            return {};
        }

        auto x_result = args[0].to_number();
        auto y_result = args[1].to_number();

        if (!std::holds_alternative<double>(x_result) || !std::holds_alternative<double>(y_result)) {
            return {};
        }

        double x = std::get<double>(x_result);
        double y = std::get<double>(y_result);
        return {runtime::Value(std::fmod(x, y))};
    }

    std::vector<runtime::Value> log(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);

        if (args.size() > 1 && args[1].is_number()) {
            auto base_result = args[1].to_number();
            if (std::holds_alternative<double>(base_result)) {
                double base = std::get<double>(base_result);
                return {runtime::Value(std::log(value) / std::log(base))};
            }
        }

        return {runtime::Value(std::log(value))};
    }

    std::vector<runtime::Value> max(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {};
        }

        double max_val = -std::numeric_limits<double>::infinity();
        bool found_number = false;

        for (const auto& arg : args) {
            if (arg.is_number()) {
                auto num_result = arg.to_number();
                if (std::holds_alternative<double>(num_result)) {
                    double val = std::get<double>(num_result);
                    if (!found_number || val > max_val) {
                        max_val = val;
                        found_number = true;
                    }
                }
            }
        }

        if (found_number) {
            return {runtime::Value(max_val)};
        }
        return {};
    }

    std::vector<runtime::Value> min(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            return {};
        }

        double min_val = std::numeric_limits<double>::infinity();
        bool found_number = false;

        for (const auto& arg : args) {
            if (arg.is_number()) {
                auto num_result = arg.to_number();
                if (std::holds_alternative<double>(num_result)) {
                    double val = std::get<double>(num_result);
                    if (!found_number || val < min_val) {
                        min_val = val;
                        found_number = true;
                    }
                }
            }
        }

        if (found_number) {
            return {runtime::Value(min_val)};
        }
        return {};
    }

    std::vector<runtime::Value> modf(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        double integral_part;
        double fractional_part = std::modf(value, &integral_part);

        return {runtime::Value(integral_part), runtime::Value(fractional_part)};
    }

    std::vector<runtime::Value> rad(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double degrees = std::get<double>(num_result);
        double radians = degrees * M_PI / 180.0;
        return {runtime::Value(radians)};
    }

    std::vector<runtime::Value> random(runtime::IVMContext* vm,
                                     const std::vector<runtime::Value>& args) {
        if (args.empty()) {
            // Return random float between 0 and 1
            return {runtime::Value(dis(gen))};
        }

        if (args.size() == 1 && args[0].is_number()) {
            auto num_result = args[0].to_number();
            if (std::holds_alternative<double>(num_result)) {
                int n = static_cast<int>(std::get<double>(num_result));
                if (n >= 1) {
                    std::uniform_int_distribution<int> int_dis(1, n);
                    return {runtime::Value(static_cast<double>(int_dis(gen)))};
                }
            }
        }

        if (args.size() >= 2 && args[0].is_number() && args[1].is_number()) {
            auto m_result = args[0].to_number();
            auto n_result = args[1].to_number();

            if (std::holds_alternative<double>(m_result) && std::holds_alternative<double>(n_result)) {
                int m = static_cast<int>(std::get<double>(m_result));
                int n = static_cast<int>(std::get<double>(n_result));

                if (m <= n) {
                    std::uniform_int_distribution<int> int_dis(m, n);
                    return {runtime::Value(static_cast<double>(int_dis(gen)))};
                }
            }
        }

        return {runtime::Value(dis(gen))};
    }

    std::vector<runtime::Value> randomseed(runtime::IVMContext* vm,
                                         const std::vector<runtime::Value>& args) {
        if (!args.empty() && args[0].is_number()) {
            auto num_result = args[0].to_number();
            if (std::holds_alternative<double>(num_result)) {
                unsigned int seed = static_cast<unsigned int>(std::get<double>(num_result));
                gen.seed(seed);
            }
        }
        return {};
    }

    std::vector<runtime::Value> sin(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::sin(value))};
    }

    std::vector<runtime::Value> sqrt(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::sqrt(value))};
    }

    std::vector<runtime::Value> tan(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {};
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {};
        }

        double value = std::get<double>(num_result);
        return {runtime::Value(std::tan(value))};
    }

    std::vector<runtime::Value> tointeger(runtime::IVMContext* vm,
                                        const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {runtime::Value()};  // nil
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {runtime::Value()};  // nil
        }

        double value = std::get<double>(num_result);

        // Check if it's already an integer
        if (value == std::floor(value) && std::isfinite(value)) {
            return {runtime::Value(value)};
        }

        return {runtime::Value()};  // nil
    }

    std::vector<runtime::Value> type(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args) {
        if (args.empty() || !args[0].is_number()) {
            return {runtime::Value()};  // nil
        }

        auto num_result = args[0].to_number();
        if (!std::holds_alternative<double>(num_result)) {
            return {runtime::Value()};  // nil
        }

        double value = std::get<double>(num_result);

        // Check if it's an integer
        if (value == std::floor(value) && std::isfinite(value)) {
            return {runtime::Value("integer")};
        } else {
            return {runtime::Value("float")};
        }
    }

    std::vector<runtime::Value> ult(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args) {
        if (args.size() < 2 || !args[0].is_number() || !args[1].is_number()) {
            return {runtime::Value(false)};
        }

        auto m_result = args[0].to_number();
        auto n_result = args[1].to_number();

        if (!std::holds_alternative<double>(m_result) || !std::holds_alternative<double>(n_result)) {
            return {runtime::Value(false)};
        }

        // Convert to unsigned integers for comparison
        unsigned long long m = static_cast<unsigned long long>(std::get<double>(m_result));
        unsigned long long n = static_cast<unsigned long long>(std::get<double>(n_result));

        return {runtime::Value(m < n)};
    }

    void register_functions(runtime::IVMContext* vm,
                              const runtime::GCPtr<runtime::Table>& globals) {
        // Create math table
        auto math_table = runtime::value_factory::table();
        auto math_table_ptr = math_table.to_table();

        if (std::holds_alternative<runtime::GCPtr<runtime::Table>>(math_table_ptr)) {
            auto table = std::get<runtime::GCPtr<runtime::Table>>(math_table_ptr);

            // Register math library functions
            table->set(runtime::Value("abs"), runtime::value_factory::function(abs, vm));
            table->set(runtime::Value("acos"), runtime::value_factory::function(acos, vm));
            table->set(runtime::Value("asin"), runtime::value_factory::function(asin, vm));
            table->set(runtime::Value("atan"), runtime::value_factory::function(atan, vm));
            table->set(runtime::Value("ceil"), runtime::value_factory::function(ceil, vm));
            table->set(runtime::Value("cos"), runtime::value_factory::function(cos, vm));
            table->set(runtime::Value("deg"), runtime::value_factory::function(deg, vm));
            table->set(runtime::Value("exp"), runtime::value_factory::function(exp, vm));
            table->set(runtime::Value("floor"), runtime::value_factory::function(floor, vm));
            table->set(runtime::Value("fmod"), runtime::value_factory::function(fmod, vm));
            table->set(runtime::Value("log"), runtime::value_factory::function(log, vm));
            table->set(runtime::Value("max"), runtime::value_factory::function(max, vm));
            table->set(runtime::Value("min"), runtime::value_factory::function(min, vm));
            table->set(runtime::Value("modf"), runtime::value_factory::function(modf, vm));
            table->set(runtime::Value("rad"), runtime::value_factory::function(rad, vm));
            table->set(runtime::Value("random"), runtime::value_factory::function(random, vm));
            table->set(runtime::Value("randomseed"),
                       runtime::value_factory::function(randomseed, vm));
            table->set(runtime::Value("sin"), runtime::value_factory::function(sin, vm));
            table->set(runtime::Value("sqrt"), runtime::value_factory::function(sqrt, vm));
            table->set(runtime::Value("tan"), runtime::value_factory::function(tan, vm));
            table->set(runtime::Value("tointeger"),
                       runtime::value_factory::function(tointeger, vm));
            table->set(runtime::Value("type"), runtime::value_factory::function(type, vm));
            table->set(runtime::Value("ult"), runtime::value_factory::function(ult, vm));

            // Add mathematical constants
            table->set(runtime::Value("pi"), runtime::Value(M_PI));
            table->set(runtime::Value("huge"), runtime::Value(std::numeric_limits<double>::infinity()));

            // Register the math table in globals
            globals->set(runtime::Value("math"), math_table);
        }
    }

}  // namespace rangelua::stdlib::math
