-- Test 6: Table length operator
print("=== Test 6: Table length operator ===")

-- Array-style table
local arr = {10, 20, 30, 40, 50}
print("Array length:", #arr)

-- Empty table
local empty = {}
print("Empty table length:", #empty)

-- Table with gaps
local gaps = {1, 2, nil, 4, 5}
print("Table with gaps length:", #gaps)

-- Mixed table (length should be array part only)
local mixed = {1, 2, 3, x = "hello", y = "world"}
print("Mixed table length:", #mixed)

-- Table with only hash part
local hash_only = {x = 1, y = 2, z = 3}
print("Hash-only table length:", #hash_only)

print("Table length tests completed")
