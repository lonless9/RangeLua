-- Test: Comprehensive bitwise operations
-- Expected output:
-- -6
-- -1
-- 6
-- 5
-- Description: Tests both unary and binary bitwise operations

-- Unary bitwise NOT
print(~5)    -- ~5 = -6 (two's complement)
print(~0)    -- ~0 = -1 (two's complement)

-- Binary bitwise XOR
print(5 ~ 3)    -- 5 XOR 3 = 6 (101 XOR 011 = 110)
print(5 ~ 0)    -- 5 XOR 0 = 5 (101 XOR 000 = 101)
