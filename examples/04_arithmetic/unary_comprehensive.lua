-- Test: Comprehensive unary operations
-- Expected output:
-- -5
-- -3.14
-- true
-- false
-- 5
-- Description: Tests all valid unary operators in Lua

-- Unary minus (arithmetic negation)
print(-5)       -- Integer negation
print(-3.14)    -- Float negation

-- Logical not
print(not false)  -- not false = true
print(not true)   -- not true = false

-- Length operator
print(#"hello")   -- Length of string = 5

-- Note: Bitwise not (~) requires integer operands and would be tested separately
-- Note: Unary plus (+) is NOT a valid operator in Lua
