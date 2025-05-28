-- Test: Comprehensive Control Flow
-- Expected output:
-- Conditional tests:
-- x is positive
-- y is zero
-- z is negative
-- Nested conditions:
-- a is positive and greater than 5
-- While loop test:
-- Count: 1
-- Count: 2
-- Count: 3
-- For numeric loop:
-- i = 1
-- i = 2
-- i = 3
-- i = 4
-- i = 5
-- Complex control flow:
-- Processing number: 1
-- 1 is odd
-- Processing number: 2
-- 2 is even
-- Processing number: 3
-- 3 is odd
-- Processing number: 4
-- 4 is even
-- Processing number: 5
-- 5 is odd
-- Description: Comprehensive test covering all modules with control flow from 06_control

-- Module 03_variables + 05_logic + 06_control: Conditional statements
print("Conditional tests:")
local x = 10
local y = 0
local z = -5

if x > 0 then
    print("x is positive")
elseif x == 0 then
    print("x is zero")
else
    print("x is negative")
end

if y > 0 then
    print("y is positive")
elseif y == 0 then
    print("y is zero")
else
    print("y is negative")
end

if z > 0 then
    print("z is positive")
elseif z == 0 then
    print("z is zero")
else
    print("z is negative")
end

-- Nested conditions
print("Nested conditions:")
local a = 8
if a > 0 then
    if a > 5 then
        print("a is positive and greater than 5")
    else
        print("a is positive but not greater than 5")
    end
else
    print("a is not positive")
end

-- Module 06_control: While loop
print("While loop test:")
local count = 1
while count <= 3 do
    print("Count:", count)
    count = count + 1
end

-- Module 06_control: For numeric loop
print("For numeric loop:")
for i = 1, 5 do
    print("i =", i)
end

-- Complex control flow combining all modules
print("Complex control flow:")
for num = 1, 5 do
    print("Processing number:", num)
    
    -- Use arithmetic and logic
    local remainder = num % 2
    if remainder == 0 then
        print(num, "is even")
    else
        print(num, "is odd")
    end
end
