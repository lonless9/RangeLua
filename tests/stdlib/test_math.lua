-- Test cases for RangeLua math library functions
-- This file tests all implemented functions in stdlib::math

print("=== RangeLua Math Library Test Suite ===")
print()

-- Test 1: Basic arithmetic functions
print("Test 1: Basic arithmetic functions")
print("math.abs(-5) =", math.abs(-5))
print("math.abs(5) =", math.abs(5))
print("math.abs(-3.14) =", math.abs(-3.14))
print("math.abs(0) =", math.abs(0))
print()

print("math.max(1, 2, 3) =", math.max(1, 2, 3))
print("math.max(-1, -2, -3) =", math.max(-1, -2, -3))
print("math.max(3.14, 2.71) =", math.max(3.14, 2.71))
print()

print("math.min(1, 2, 3) =", math.min(1, 2, 3))
print("math.min(-1, -2, -3) =", math.min(-1, -2, -3))
print("math.min(3.14, 2.71) =", math.min(3.14, 2.71))
print()

-- Test 2: Rounding functions
print("Test 2: Rounding functions")
print("math.floor(3.7) =", math.floor(3.7))
print("math.floor(-3.7) =", math.floor(-3.7))
print("math.floor(5) =", math.floor(5))
print()

print("math.ceil(3.2) =", math.ceil(3.2))
print("math.ceil(-3.2) =", math.ceil(-3.2))
print("math.ceil(5) =", math.ceil(5))
print()

-- Test 3: Exponential and logarithmic functions
print("Test 3: Exponential and logarithmic functions")
print("math.exp(0) =", math.exp(0))
print("math.exp(1) =", math.exp(1))
print("math.exp(2) =", math.exp(2))
print()

print("math.log(1) =", math.log(1))
print("math.log(math.exp(1)) =", math.log(math.exp(1)))
print("math.log(10) =", math.log(10))
print("math.log(100, 10) =", math.log(100, 10))
print()

print("math.sqrt(4) =", math.sqrt(4))
print("math.sqrt(9) =", math.sqrt(9))
print("math.sqrt(2) =", math.sqrt(2))
print("math.sqrt(0) =", math.sqrt(0))
print()

-- Test 4: Trigonometric functions
print("Test 4: Trigonometric functions")
print("math.sin(0) =", math.sin(0))
print("math.sin(math.pi/2) =", math.sin(math.pi/2))
print("math.sin(math.pi) =", math.sin(math.pi))
print()

print("math.cos(0) =", math.cos(0))
print("math.cos(math.pi/2) =", math.cos(math.pi/2))
print("math.cos(math.pi) =", math.cos(math.pi))
print()

print("math.tan(0) =", math.tan(0))
print("math.tan(math.pi/4) =", math.tan(math.pi/4))
print()

-- Test 5: Inverse trigonometric functions
print("Test 5: Inverse trigonometric functions")
print("math.asin(0) =", math.asin(0))
print("math.asin(1) =", math.asin(1))
print("math.asin(-1) =", math.asin(-1))
print()

print("math.acos(0) =", math.acos(0))
print("math.acos(1) =", math.acos(1))
print("math.acos(-1) =", math.acos(-1))
print()

print("math.atan(0) =", math.atan(0))
print("math.atan(1) =", math.atan(1))
print("math.atan(-1) =", math.atan(-1))
print()

-- Test 6: Angle conversion
print("Test 6: Angle conversion")
print("math.deg(math.pi) =", math.deg(math.pi))
print("math.deg(math.pi/2) =", math.deg(math.pi/2))
print("math.deg(0) =", math.deg(0))
print()

print("math.rad(180) =", math.rad(180))
print("math.rad(90) =", math.rad(90))
print("math.rad(0) =", math.rad(0))
print()

-- Test 7: Modular arithmetic
print("Test 7: Modular arithmetic")
print("math.fmod(7, 3) =", math.fmod(7, 3))
print("math.fmod(-7, 3) =", math.fmod(-7, 3))
print("math.fmod(7, -3) =", math.fmod(7, -3))
print("math.fmod(5.5, 2) =", math.fmod(5.5, 2))
print()

-- Test 8: modf function
print("Test 8: modf function")
local i, f = math.modf(3.14)
print("math.modf(3.14) = integer:", i, "fraction:", f)
i, f = math.modf(-3.14)
print("math.modf(-3.14) = integer:", i, "fraction:", f)
i, f = math.modf(5)
print("math.modf(5) = integer:", i, "fraction:", f)
print()

-- Test 9: Random number generation
print("Test 9: Random number generation")
math.randomseed(12345)  -- Set seed for reproducible results
print("math.random() =", math.random())
print("math.random() =", math.random())
print("math.random(10) =", math.random(10))
print("math.random(5, 15) =", math.random(5, 15))
print("math.random(1, 6) =", math.random(1, 6))  -- Dice roll
print()

-- Test 10: Type checking and conversion
print("Test 10: Type checking and conversion")
print("math.type(42) =", math.type(42))
print("math.type(3.14) =", math.type(3.14))
print("math.type('hello') =", math.type('hello'))
print()

print("math.tointeger(42.0) =", math.tointeger(42.0))
print("math.tointeger(42.7) =", math.tointeger(42.7))
print("math.tointeger('42') =", math.tointeger('42'))
print("math.tointeger('hello') =", math.tointeger('hello'))
print()

-- Test 11: Unsigned integer comparison
print("Test 11: Unsigned integer comparison")
print("math.ult(2, 3) =", math.ult(2, 3))
print("math.ult(3, 2) =", math.ult(3, 2))
print("math.ult(-1, 1) =", math.ult(-1, 1))  -- -1 as unsigned is very large
print()

-- Test 12: Mathematical constants
print("Test 12: Mathematical constants")
print("math.pi =", math.pi)
print("math.huge =", math.huge)
print("math.mininteger =", math.mininteger)
print("math.maxinteger =", math.maxinteger)
print()

-- Test 13: Complex calculations
print("Test 13: Complex calculations")
-- Calculate area of circle
local radius = 5
local area = math.pi * radius * radius
print("Area of circle with radius", radius, "=", area)

-- Calculate hypotenuse
local a, b = 3, 4
local c = math.sqrt(a*a + b*b)
print("Hypotenuse of triangle with sides", a, "and", b, "=", c)

-- Calculate compound interest
local principal = 1000
local rate = 0.05
local time = 10
local amount = principal * math.exp(rate * time)
print("Compound interest: $" .. string.format("%.2f", amount))
print()

-- Test 14: Edge cases and special values
print("Test 14: Edge cases and special values")
print("math.sqrt(-1) =", math.sqrt(-1))  -- Should be NaN
print("math.log(0) =", math.log(0))      -- Should be -inf
print("math.log(-1) =", math.log(-1))    -- Should be NaN
print("1/0 =", 1/0)                      -- Should be inf
print("-1/0 =", -1/0)                    -- Should be -inf
print("0/0 =", 0/0)                      -- Should be NaN
print()

print("=== Math Library Tests Complete ===")
