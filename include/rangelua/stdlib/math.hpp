#pragma once

/**
 * @file math.hpp
 * @brief Lua math library implementation for RangeLua
 * @version 0.1.0
 *
 * Implements the Lua math library functions including mathematical operations,
 * trigonometric functions, random number generation, and other mathematical utilities.
 */

#include <vector>

#include "../runtime/value.hpp"

namespace rangelua::stdlib::math {

    /**
     * @brief Lua math.abs function implementation
     *
     * Returns the absolute value of x.
     *
     * @param args Vector containing the number
     * @return Vector containing the absolute value
     */
    std::vector<runtime::Value> abs(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.acos function implementation
     *
     * Returns the arc cosine of x (in radians).
     *
     * @param args Vector containing the number
     * @return Vector containing the arc cosine
     */
    std::vector<runtime::Value> acos(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.asin function implementation
     *
     * Returns the arc sine of x (in radians).
     *
     * @param args Vector containing the number
     * @return Vector containing the arc sine
     */
    std::vector<runtime::Value> asin(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.atan function implementation
     *
     * Returns the arc tangent of x (in radians).
     *
     * @param args Vector containing the number(s)
     * @return Vector containing the arc tangent
     */
    std::vector<runtime::Value> atan(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.ceil function implementation
     *
     * Returns the smallest integer larger than or equal to x.
     *
     * @param args Vector containing the number
     * @return Vector containing the ceiling
     */
    std::vector<runtime::Value> ceil(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.cos function implementation
     *
     * Returns the cosine of x (assumed to be in radians).
     *
     * @param args Vector containing the number
     * @return Vector containing the cosine
     */
    std::vector<runtime::Value> cos(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.deg function implementation
     *
     * Converts angle x from radians to degrees.
     *
     * @param args Vector containing the angle in radians
     * @return Vector containing the angle in degrees
     */
    std::vector<runtime::Value> deg(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.exp function implementation
     *
     * Returns e^x.
     *
     * @param args Vector containing the exponent
     * @return Vector containing e^x
     */
    std::vector<runtime::Value> exp(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.floor function implementation
     *
     * Returns the largest integer smaller than or equal to x.
     *
     * @param args Vector containing the number
     * @return Vector containing the floor
     */
    std::vector<runtime::Value> floor(runtime::IVMContext* vm,
                                    const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.fmod function implementation
     *
     * Returns the remainder of the division of x by y.
     *
     * @param args Vector containing x and y
     * @return Vector containing x % y
     */
    std::vector<runtime::Value> fmod(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.log function implementation
     *
     * Returns the logarithm of x in the given base.
     *
     * @param args Vector containing x and optional base
     * @return Vector containing the logarithm
     */
    std::vector<runtime::Value> log(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.max function implementation
     *
     * Returns the maximum value among its arguments.
     *
     * @param args Vector containing the numbers
     * @return Vector containing the maximum
     */
    std::vector<runtime::Value> max(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.min function implementation
     *
     * Returns the minimum value among its arguments.
     *
     * @param args Vector containing the numbers
     * @return Vector containing the minimum
     */
    std::vector<runtime::Value> min(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.modf function implementation
     *
     * Returns the integral and fractional parts of x.
     *
     * @param args Vector containing the number
     * @return Vector containing integral and fractional parts
     */
    std::vector<runtime::Value> modf(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.rad function implementation
     *
     * Converts angle x from degrees to radians.
     *
     * @param args Vector containing the angle in degrees
     * @return Vector containing the angle in radians
     */
    std::vector<runtime::Value> rad(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.random function implementation
     *
     * Returns a pseudo-random number.
     *
     * @param args Vector containing optional range parameters
     * @return Vector containing the random number
     */
    std::vector<runtime::Value> random(runtime::IVMContext* vm,
                                     const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.randomseed function implementation
     *
     * Sets the seed for the pseudo-random generator.
     *
     * @param args Vector containing the seed
     * @return Vector (empty)
     */
    std::vector<runtime::Value> randomseed(runtime::IVMContext* vm,
                                         const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.sin function implementation
     *
     * Returns the sine of x (assumed to be in radians).
     *
     * @param args Vector containing the number
     * @return Vector containing the sine
     */
    std::vector<runtime::Value> sin(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.sqrt function implementation
     *
     * Returns the square root of x.
     *
     * @param args Vector containing the number
     * @return Vector containing the square root
     */
    std::vector<runtime::Value> sqrt(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.tan function implementation
     *
     * Returns the tangent of x (assumed to be in radians).
     *
     * @param args Vector containing the number
     * @return Vector containing the tangent
     */
    std::vector<runtime::Value> tan(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.tointeger function implementation
     *
     * If the value x is convertible to an integer, returns that integer.
     *
     * @param args Vector containing the value
     * @return Vector containing the integer or nil
     */
    std::vector<runtime::Value> tointeger(runtime::IVMContext* vm,
                                        const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.type function implementation
     *
     * Returns "integer" if x is an integer, "float" if it is a float, or nil if x is not a number.
     *
     * @param args Vector containing the value
     * @return Vector containing the type string or nil
     */
    std::vector<runtime::Value> type(runtime::IVMContext* vm,
                                   const std::vector<runtime::Value>& args);

    /**
     * @brief Lua math.ult function implementation
     *
     * Returns true if integer m is below integer n when they are compared as unsigned integers.
     *
     * @param args Vector containing m and n
     * @return Vector containing boolean result
     */
    std::vector<runtime::Value> ult(runtime::IVMContext* vm,
                                  const std::vector<runtime::Value>& args);

    /**
     * @brief Register math library functions in the given global table
     *
     * This function creates the math table and registers all math library
     * functions in it, then adds it to the global environment.
     *
     * @param globals The global environment table to register the math library in
     */
    void register_functions(runtime::IVMContext* vm,
                              const runtime::GCPtr<runtime::Table>& globals);

}  // namespace rangelua::stdlib::math
