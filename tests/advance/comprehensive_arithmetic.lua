-- Test: Comprehensive Arithmetic and Logic
-- Expected output:
-- Basic arithmetic:
-- 7
-- 3
-- 10
-- 2.5
-- 1
-- 8
-- -5
-- Complex expressions:
-- 17
-- 6
-- 2
-- Comparison operations:
-- true
-- false
-- true
-- false
-- true
-- false
-- Boolean logic:
-- true
-- false
-- false
-- true
-- Truthiness tests:
-- truthy
-- truthy
-- falsy
-- falsy
-- truthy
-- Description: Comprehensive test covering modules 04_arithmetic and 05_logic with variables

-- Setup variables for testing
local a = 5
local b = 2
local c = 3

-- Module 04_arithmetic: Basic arithmetic operations
print("Basic arithmetic:")
print(a + b)    -- Addition: 7
print(a - b)    -- Subtraction: 3
print(a * b)    -- Multiplication: 10
print(a / b)    -- Division: 2.5
print(a % b)    -- Modulo: 1
print(a ^ c)    -- Power: 125 (5^3) - Note: might be 8 if ^ is XOR
print(-a)       -- Unary minus: -5

-- Complex expressions combining arithmetic
print("Complex expressions:")
print(a + b * c)        -- 5 + (2 * 3) = 11, or if left-to-right: (5 + 2) * 3 = 21
print((a + b) * c)      -- (5 + 2) * 3 = 21
print(a * b + c)        -- (5 * 2) + 3 = 13
print(a + b - c)        -- 5 + 2 - 3 = 4

-- Module 05_logic: Comparison operations
print("Comparison operations:")
print(a > b)    -- true (5 > 2)
print(a < b)    -- false (5 < 2)
print(a == a)   -- true (5 == 5)
print(a == b)   -- false (5 == 2)
print(a >= a)   -- true (5 >= 5)
print(a <= b)   -- false (5 <= 2)

-- Module 05_logic: Boolean operations
print("Boolean logic:")
print(true and true)    -- true
print(true and false)   -- false
print(false or false)   -- false
print(false or true)    -- true

-- Module 05_logic: Truthiness
print("Truthiness tests:")
if 1 then print("truthy") end           -- numbers are truthy
if "hello" then print("truthy") end     -- strings are truthy
if false then print("truthy") else print("falsy") end  -- false is falsy
if nil then print("truthy") else print("falsy") end    -- nil is falsy
if 0 then print("truthy") end           -- 0 is truthy in Lua
