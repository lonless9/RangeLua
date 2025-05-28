-- Test: Comprehensive Mixed Functionality
-- Expected output:
-- Calculator Program
-- =================
-- Initial values: a=10, b=5
-- Addition: 10 + 5 = 15
-- Subtraction: 10 - 5 = 5
-- Multiplication: 10 * 5 = 50
-- Division: 10 / 5 = 2
-- 
-- Comparison Results:
-- 10 > 5: true
-- 10 == 5: false
-- 10 <= 5: false
-- 
-- Loop Calculations:
-- Square of 1 = 1
-- Square of 2 = 4
-- Square of 3 = 9
-- Square of 4 = 16
-- Square of 5 = 25
-- 
-- Conditional Logic:
-- Testing value: 1 (odd, small)
-- Testing value: 2 (even, small)
-- Testing value: 3 (odd, small)
-- Testing value: 4 (even, small)
-- Testing value: 5 (odd, small)
-- Testing value: 6 (even, medium)
-- Testing value: 7 (odd, medium)
-- Testing value: 8 (even, medium)
-- Testing value: 9 (odd, medium)
-- Testing value: 10 (even, large)
-- 
-- Type Analysis:
-- Value: hello, Type: string, Length check: long enough
-- Value: 42, Type: number, Sign: positive
-- Value: true, Type: boolean, Truth: is true
-- Value: nil, Type: nil, Existence: does not exist
-- 
-- Summary: All tests completed successfully!
-- Description: Complex integration test using all modules in realistic scenarios

-- Complex program demonstrating all functionality
print("Calculator Program")
print("=================")

-- Initialize variables (Module 03)
local a = 10
local b = 5
print("Initial values: a=" .. a .. ", b=" .. b)

-- Arithmetic operations (Module 04)
local sum = a + b
local diff = a - b
local product = a * b
local quotient = a / b

print("Addition: " .. a .. " + " .. b .. " = " .. sum)
print("Subtraction: " .. a .. " - " .. b .. " = " .. diff)
print("Multiplication: " .. a .. " * " .. b .. " = " .. product)
print("Division: " .. a .. " / " .. b .. " = " .. quotient)
print("")

-- Comparison operations (Module 05)
print("Comparison Results:")
print(a .. " > " .. b .. ": " .. tostring(a > b))
print(a .. " == " .. b .. ": " .. tostring(a == b))
print(a .. " <= " .. b .. ": " .. tostring(a <= b))
print("")

-- Loop with calculations (Module 06)
print("Loop Calculations:")
for i = 1, 5 do
    local square = i * i
    print("Square of " .. i .. " = " .. square)
end
print("")

-- Complex conditional logic (All modules)
print("Conditional Logic:")
for value = 1, 10 do
    local description = "Testing value: " .. value
    
    -- Check if even or odd
    if value % 2 == 0 then
        description = description .. " (even"
    else
        description = description .. " (odd"
    end
    
    -- Check size category
    if value <= 5 then
        description = description .. ", small)"
    elseif value <= 8 then
        description = description .. ", medium)"
    else
        description = description .. ", large)"
    end
    
    print(description)
end
print("")

-- Type checking and analysis (Module 02)
print("Type Analysis:")
local test_values = {"hello", 42, true, nil}
local test_names = {"hello", "42", "true", "nil"}

for i = 1, 4 do
    local val = test_values[i]
    local name = test_names[i]
    local val_type = type(val)
    
    local analysis = "Value: " .. name .. ", Type: " .. val_type
    
    if val_type == "string" then
        if string.len(val) > 3 then
            analysis = analysis .. ", Length check: long enough"
        else
            analysis = analysis .. ", Length check: too short"
        end
    elseif val_type == "number" then
        if val > 0 then
            analysis = analysis .. ", Sign: positive"
        elseif val < 0 then
            analysis = analysis .. ", Sign: negative"
        else
            analysis = analysis .. ", Sign: zero"
        end
    elseif val_type == "boolean" then
        if val then
            analysis = analysis .. ", Truth: is true"
        else
            analysis = analysis .. ", Truth: is false"
        end
    elseif val_type == "nil" then
        analysis = analysis .. ", Existence: does not exist"
    end
    
    print(analysis)
end
print("")

print("Summary: All tests completed successfully!")
