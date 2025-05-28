-- Test 7: Table iteration patterns
print("=== Test 7: Table iteration patterns ===")

-- Test table
local t = {
    [1] = "first",
    [2] = "second", 
    [3] = "third",
    name = "test table",
    value = 42
}

-- Test ipairs (if implemented)
print("Testing ipairs:")
for i, v in ipairs(t) do
    print("  ipairs:", i, v)
end

-- Test pairs (if implemented)
print("Testing pairs:")
for k, v in pairs(t) do
    print("  pairs:", k, v)
end

print("Table iteration tests completed")
