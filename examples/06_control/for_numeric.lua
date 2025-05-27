-- Test: Numeric for loops
-- Expected output:
-- 1
-- 2
-- 3
-- 4
-- 5
-- 10
-- 8
-- 6
-- 4
-- 2
-- Description: Tests numeric for loops with different step values

-- Basic for loop (step = 1)
for i = 1, 5 do
    print(i)
end

-- For loop with step = -2
for i = 10, 2, -2 do
    print(i)
end
