-- Test 3: Table literal initialization
print("=== Test 3: Table literal initialization ===")

-- Array-style initialization
local numbers = {1, 2, 3, 4, 5}
print("Array literal created")

-- Mixed initialization
local mixed = {
    10,           -- [1] = 10
    20,           -- [2] = 20
    x = "hello",  -- x = "hello"
    y = "world",  -- y = "world"
    [5] = "five", -- [5] = "five"
    30            -- [3] = 30
}
print("Mixed literal created")

-- Test access
print("numbers[1] =", numbers[1])
print("numbers[3] =", numbers[3])
print("mixed[1] =", mixed[1])
print("mixed.x =", mixed.x)
print("mixed[5] =", mixed[5])

print("Table literal tests completed")
