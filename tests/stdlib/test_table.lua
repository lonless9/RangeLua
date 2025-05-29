-- Test cases for RangeLua table library functions
-- This file tests all implemented functions in stdlib::table

print("=== RangeLua Table Library Test Suite ===")
print()

-- Test 1: table.concat function
print("Test 1: table.concat function")
local t1 = {"hello", "world", "lua", "programming"}
print("table.concat(t1) =", table.concat(t1))
print("table.concat(t1, ' ') =", table.concat(t1, ' '))
print("table.concat(t1, '-') =", table.concat(t1, '-'))
print("table.concat(t1, ', ', 2, 3) =", table.concat(t1, ', ', 2, 3))

local t2 = {1, 2, 3, 4, 5}
print("table.concat(t2, '+') =", table.concat(t2, '+'))
print()

-- Test 2: table.insert function
print("Test 2: table.insert function")
local t3 = {1, 2, 3}
print("Original table:", table.concat(t3, ', '))

table.insert(t3, 4)
print("After insert(t3, 4):", table.concat(t3, ', '))

table.insert(t3, 2, 1.5)
print("After insert(t3, 2, 1.5):", table.concat(t3, ', '))

table.insert(t3, 1, 0)
print("After insert(t3, 1, 0):", table.concat(t3, ', '))
print()

-- Test 3: table.remove function
print("Test 3: table.remove function")
local t4 = {10, 20, 30, 40, 50}
print("Original table:", table.concat(t4, ', '))

local removed = table.remove(t4)
print("After remove():", table.concat(t4, ', '), "removed:", removed)

removed = table.remove(t4, 2)
print("After remove(t4, 2):", table.concat(t4, ', '), "removed:", removed)

removed = table.remove(t4, 1)
print("After remove(t4, 1):", table.concat(t4, ', '), "removed:", removed)
print()

-- Test 4: table.pack function
print("Test 4: table.pack function")
local packed = table.pack("a", "b", "c", "d")
print("table.pack('a', 'b', 'c', 'd'):")
print("  n =", packed.n)
for i = 1, packed.n do
    print("  [" .. i .. "] =", packed[i])
end

local packed2 = table.pack(1, nil, 3, nil, 5)
print("table.pack(1, nil, 3, nil, 5):")
print("  n =", packed2.n)
for i = 1, packed2.n do
    print("  [" .. i .. "] =", packed2[i])
end
print()

-- Test 5: table.unpack function
print("Test 5: table.unpack function")
local t5 = {"x", "y", "z"}
print("table.unpack(t5) =", table.unpack(t5))
print("table.unpack(t5, 2) =", table.unpack(t5, 2))
print("table.unpack(t5, 1, 2) =", table.unpack(t5, 1, 2))

local t6 = {10, 20, 30, 40, 50}
print("table.unpack(t6, 2, 4) =", table.unpack(t6, 2, 4))
print()

-- Test 6: table.sort function
print("Test 6: table.sort function")
local t7 = {5, 2, 8, 1, 9, 3}
print("Original:", table.concat(t7, ', '))
table.sort(t7)
print("After sort():", table.concat(t7, ', '))

local t8 = {"banana", "apple", "cherry", "date"}
print("Original strings:", table.concat(t8, ', '))
table.sort(t8)
print("After sort():", table.concat(t8, ', '))

-- Sort with custom comparison function
local t9 = {5, 2, 8, 1, 9, 3}
table.sort(t9, function(a, b) return a > b end)  -- Descending order
print("Descending sort:", table.concat(t9, ', '))
print()

-- Test 7: table.move function
print("Test 7: table.move function")
local source = {10, 20, 30, 40, 50}
local dest = {1, 2, 3, 4, 5, 6, 7}
print("Source:", table.concat(source, ', '))
print("Dest before:", table.concat(dest, ', '))

table.move(source, 2, 4, 3, dest)
print("After move(source, 2, 4, 3, dest):", table.concat(dest, ', '))

-- Move within same table
local t10 = {1, 2, 3, 4, 5, 6}
print("Before move within same table:", table.concat(t10, ', '))
table.move(t10, 1, 3, 4)
print("After move(t10, 1, 3, 4):", table.concat(t10, ', '))
print()

-- Test 8: Complex table operations
print("Test 8: Complex table operations")
local data = {"apple", "banana", "cherry", "date", "elderberry"}
print("Original data:", table.concat(data, ', '))

-- Insert at beginning and end
table.insert(data, 1, "apricot")
table.insert(data, "fig")
print("After insertions:", table.concat(data, ', '))

-- Remove from middle
table.remove(data, 3)
print("After removal:", table.concat(data, ', '))

-- Sort alphabetically
table.sort(data)
print("After sorting:", table.concat(data, ', '))
print()

-- Test 9: Table with mixed types
print("Test 9: Table with mixed types")
local mixed = {1, "hello", 3.14, true, nil, "world"}
local packed_mixed = table.pack(table.unpack(mixed))
print("Mixed table pack/unpack:")
print("  n =", packed_mixed.n)
for i = 1, packed_mixed.n do
    print("  [" .. i .. "] =", tostring(packed_mixed[i]))
end
print()

-- Test 10: Edge cases
print("Test 10: Edge cases")
-- Empty table operations
local empty = {}
print("Empty table concat:", "'" .. table.concat(empty) .. "'")
print("Empty table pack n:", table.pack(table.unpack(empty)).n)

-- Single element table
local single = {"only"}
print("Single element concat:", table.concat(single))
table.sort(single)
print("Single element after sort:", table.concat(single))

-- Table with holes
local holes = {1, nil, 3, nil, 5}
print("Table with holes concat:", table.concat(holes, ','))
print()

-- Test 11: Performance test with larger table
print("Test 11: Performance test")
local large = {}
for i = 1, 100 do
    table.insert(large, math.random(1, 1000))
end
print("Created table with 100 random elements")
print("First 10 elements:", table.concat({table.unpack(large, 1, 10)}, ', '))

table.sort(large)
print("First 10 after sort:", table.concat({table.unpack(large, 1, 10)}, ', '))
print("Last 10 after sort:", table.concat({table.unpack(large, 91, 100)}, ', '))
print()

-- Test 12: Table library integration
print("Test 12: Table library integration")
local words = {"the", "quick", "brown", "fox"}
local sentence = table.concat(words, " ")
print("Sentence:", sentence)

-- Reverse the words
local reversed = {}
for i = #words, 1, -1 do
    table.insert(reversed, words[i])
end
print("Reversed:", table.concat(reversed, " "))

-- Sort by length
table.sort(words, function(a, b) return string.len(a) < string.len(b) end)
print("Sorted by length:", table.concat(words, " "))
print()

print("=== Table Library Tests Complete ===")
