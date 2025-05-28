-- Test: Comprehensive Edge Cases and Boundary Conditions
-- Expected output:
-- Edge Case Testing
-- =================
-- 
-- Arithmetic Edge Cases:
-- Division by small number: 1000000
-- Large number arithmetic: 2000000
-- Zero operations: 0 0 0
-- Negative number operations: -15 5 -50 -2
-- 
-- Boolean Logic Edge Cases:
-- Complex boolean: false
-- Short circuit AND: first condition false
-- Short circuit OR: second condition true
-- 
-- Variable Scope Edge Cases:
-- Global x: 100
-- Local x in block: 200
-- Global x after block: 100
-- 
-- Control Flow Edge Cases:
-- Empty loop completed
-- Single iteration: 1
-- Nested loop: 1,1 1,2 2,1 2,2
-- 
-- Type Conversion Edge Cases:
-- String to number context: 15
-- Number to string context: Value: 42
-- Boolean in arithmetic: 1 0
-- 
-- Boundary Value Tests:
-- Zero boundary: exactly zero
-- Negative boundary: negative
-- Large number: very large
-- 
-- Error Handling Tests:
-- Nil access handled
-- Division by zero: inf
-- 
-- All edge case tests completed!
-- Description: Tests edge cases, boundary conditions, and potential error scenarios

print("Edge Case Testing")
print("=================")
print("")

-- Arithmetic edge cases
print("Arithmetic Edge Cases:")
local very_small = 0.000001
local very_large = 1000000
print("Division by small number:", very_large / very_small)
print("Large number arithmetic:", very_large + very_large)

-- Zero operations
local zero = 0
print("Zero operations:", zero + zero, zero - zero, zero * zero)

-- Negative number operations
local neg_a = -10
local neg_b = -5
print("Negative number operations:", neg_a + neg_b, neg_a - neg_b, neg_a * neg_b, neg_a / neg_b)
print("")

-- Boolean logic edge cases
print("Boolean Logic Edge Cases:")
local complex_bool = (true and false) or (false and true)
print("Complex boolean:", complex_bool)

-- Short-circuit evaluation
local first_false = false
if first_false and print("This should not print") then
    print("Should not reach here")
else
    print("Short circuit AND: first condition false")
end

local first_true = true
if first_true or print("This should not print") then
    print("Short circuit OR: second condition true")
end
print("")

-- Variable scope edge cases
print("Variable Scope Edge Cases:")
local x = 100
print("Global x:", x)

do
    local x = 200
    print("Local x in block:", x)
end

print("Global x after block:", x)
print("")

-- Control flow edge cases
print("Control Flow Edge Cases:")

-- Empty loop
for i = 1, 0 do
    print("This should not print")
end
print("Empty loop completed")

-- Single iteration
for i = 5, 5 do
    print("Single iteration:", i)
end

-- Nested loops
print("Nested loop:", end="")
for i = 1, 2 do
    for j = 1, 2 do
        print(i .. "," .. j, end=" ")
    end
end
print("")
print("")

-- Type conversion edge cases
print("Type Conversion Edge Cases:")
local str_num = "10"
local num_val = 5
-- Note: Lua doesn't automatically convert strings to numbers in arithmetic
-- This might cause an error or unexpected behavior
print("String to number context:", tonumber(str_num) + num_val)

local num_str = 42
print("Number to string context: Value:", num_str)

-- Boolean in arithmetic context
local bool_true = true
local bool_false = false
print("Boolean in arithmetic:", (bool_true and 1 or 0), (bool_false and 1 or 0))
print("")

-- Boundary value tests
print("Boundary Value Tests:")
local boundary_val = 0
if boundary_val == 0 then
    print("Zero boundary: exactly zero")
elseif boundary_val > 0 then
    print("Positive boundary: positive")
else
    print("Negative boundary: negative")
end

local large_val = 999999
if large_val > 100000 then
    print("Large number: very large")
end
print("")

-- Error handling tests
print("Error Handling Tests:")
local nil_var = nil
if nil_var then
    print("Nil is truthy")
else
    print("Nil access handled")
end

-- Division by zero (might produce inf or error)
local div_zero = 1 / 0
print("Division by zero:", div_zero)
print("")

print("All edge case tests completed!")
