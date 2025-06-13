-- Test: Arithmetic operations and precedence
-- Expected output:
-- 15
-- 5
-- 50
-- 2
-- 2.5
-- 1000
-- 35
-- -5

local a = 10
local b = 5

print(a + b)
print(a - b)
print(a * b)
print(a / b)
print(b / 2)
print(a ^ 3)

-- Precedence test
print(a + b * 5)

-- Unary minus
print(-b)
