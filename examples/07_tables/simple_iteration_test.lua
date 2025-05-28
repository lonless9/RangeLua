-- Simple test to verify table iteration structure
print("=== Simple Table Iteration Test ===")

local t = {
    [1] = "first",
    [2] = "second", 
    [3] = "third"
}

print("Table created with 3 elements")

-- Test if ipairs function exists and can be called
local iter_func = ipairs(t)
print("ipairs function called successfully")
print("Iterator function type:", type(iter_func))

-- Test if pairs function exists and can be called  
local pairs_func = pairs(t)
print("pairs function called successfully")
print("Pairs function type:", type(pairs_func))

print("Basic iteration test completed")
