-- Test function and table integration
-- This script tests the association between global Table and Function objects

-- Test 1: Basic function storage and retrieval
print("=== Test 1: Basic Function Storage ===")

-- Test that print function is available in global table
print("Testing print function availability:")
print("Type of print:", type(print))

-- Test that we can call print
print("Calling print function:", "Hello, World!")

-- Test 2: Function storage in tables
print("\n=== Test 2: Function Storage in Tables ===")

-- Create a table to store functions
local function_table = {}

-- Store the print function in our table
function_table.my_print = print
function_table.my_type = type

-- Test that we can retrieve and call functions from table
print("Calling print from table:")
function_table.my_print("This is printed via table reference")

print("Calling type from table:")
print("Type of number 42:", function_table.my_type(42))

-- Test 3: Creating and storing custom functions
print("\n=== Test 3: Custom Function Storage ===")

-- Define a simple function
local function add(a, b)
    return a + b
end

-- Store it in a table
local math_functions = {}
math_functions.add = add

-- Test calling the stored function
print("Testing custom function storage:")
print("5 + 3 =", math_functions.add(5, 3))

-- Test 4: Global function registration
print("\n=== Test 4: Global Function Registration ===")

-- Define a global function
function greet(name)
    print("Hello, " .. (name or "World") .. "!")
end

-- Test that it's available globally
print("Testing global function:")
greet("RangeLua")

-- Test that it's in the global table
print("Type of greet function:", type(greet))

-- Test 5: Function as table values
print("\n=== Test 5: Functions as Table Values ===")

-- Create a table with function values
local operations = {
    add = function(x, y) return x + y end,
    multiply = function(x, y) return x * y end,
    greet = function(name) return "Hello, " .. name end
}

-- Test calling functions from table
print("2 + 3 =", operations.add(2, 3))
print("4 * 5 =", operations.multiply(4, 5))
print("Greeting:", operations.greet("User"))

-- Test 6: Nested function storage
print("\n=== Test 6: Nested Function Storage ===")

local nested = {
    math = {
        basic = {
            add = function(a, b) return a + b end,
            subtract = function(a, b) return a - b end
        }
    }
}

print("Nested function call: 10 - 3 =", nested.math.basic.subtract(10, 3))

print("\n=== All Function-Table Integration Tests Complete ===")
