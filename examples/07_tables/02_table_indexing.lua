-- Test 2: Table indexing operations
print("=== Test 2: Table indexing operations ===")

-- Create table and test assignment
local t = {}
t[1] = "first"
t[2] = "second"
t["key"] = "value"
print("Table assignments completed")

-- Test retrieval
print("t[1] =", t[1])
print("t[2] =", t[2])
print("t['key'] =", t["key"])

-- Test dot notation
local obj = {}
obj.name = "test"
obj.value = 42
print("obj.name =", obj.name)
print("obj.value =", obj.value)

print("Table indexing tests completed")
