-- Test 5: Nested table structures
print("=== Test 5: Nested table structures ===")

-- Simple nested table
local nested = {
    inner = {
        value = 42,
        name = "inner table"
    }
}
print("Nested table created")

-- Test access
print("nested.inner.value =", nested.inner.value)
print("nested.inner.name =", nested.inner.name)

-- Multi-level nesting
local deep = {
    level1 = {
        level2 = {
            level3 = {
                value = "deep value"
            }
        }
    }
}
print("Deep nested table created")
print("deep.level1.level2.level3.value =", deep.level1.level2.level3.value)

-- Array of tables
local array_of_tables = {
    {name = "first", value = 1},
    {name = "second", value = 2},
    {name = "third", value = 3}
}
print("Array of tables created")
print("array_of_tables[1].name =", array_of_tables[1].name)
print("array_of_tables[2].value =", array_of_tables[2].value)

print("Nested table tests completed")
