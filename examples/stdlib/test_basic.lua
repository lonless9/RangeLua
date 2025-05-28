-- Test cases for RangeLua basic library functions
-- This file tests all implemented functions in stdlib::basic

print("=== RangeLua Basic Library Test Suite ===")
print()

-- Test 1: print function
print("Test 1: print function")
print("Hello, World!")
print("Multiple", "arguments", "with", "tabs")
print(42, true, nil, "mixed types")
print()  -- Empty print
print()

-- Test 2: type function
print("Test 2: type function")
print("type(42) =", type(42))
print("type(3.14) =", type(3.14))
print("type('hello') =", type("hello"))
print("type(true) =", type(true))
print("type(false) =", type(false))
print("type(nil) =", type(nil))
print("type({}) =", type({}))
print("type(print) =", type(print))
print()

-- Test 3: tostring function
print("Test 3: tostring function")
print("tostring(42) =", tostring(42))
print("tostring(3.14) =", tostring(3.14))
print("tostring(true) =", tostring(true))
print("tostring(false) =", tostring(false))
print("tostring(nil) =", tostring(nil))
print("tostring('hello') =", tostring("hello"))
print()

-- Test 4: tonumber function
print("Test 4: tonumber function")
print("tonumber('42') =", tonumber("42"))
print("tonumber('3.14') =", tonumber("3.14"))
print("tonumber('0xFF') =", tonumber("0xFF"))
print("tonumber('invalid') =", tonumber("invalid"))
print("tonumber(42) =", tonumber(42))
print()

-- Test 5: Basic table operations for pairs/ipairs
print("Test 5: Table creation and basic operations")
local t1 = {10, 20, 30, 40}
local t2 = {a = 1, b = 2, c = 3}
local t3 = {1, 2, a = "hello", b = "world", 3, 4}

print("Array table t1:", tostring(t1))
print("Hash table t2:", tostring(t2))
print("Mixed table t3:", tostring(t3))
print()

-- Test 6: ipairs function
print("Test 6: ipairs function")
print("ipairs on array table:")
for i, v in ipairs(t1) do
    print("  [" .. tostring(i) .. "] = " .. tostring(v))
end

print("ipairs on mixed table:")
for i, v in ipairs(t3) do
    print("  [" .. tostring(i) .. "] = " .. tostring(v))
end
print()

-- Test 7: pairs function  
print("Test 7: pairs function")
print("pairs on hash table:")
for k, v in pairs(t2) do
    print("  [" .. tostring(k) .. "] = " .. tostring(v))
end

print("pairs on mixed table:")
for k, v in pairs(t3) do
    print("  [" .. tostring(k) .. "] = " .. tostring(v))
end
print()

-- Test 8: next function
print("Test 8: next function")
print("Manual iteration with next:")
local k, v = next(t2, nil)
while k ~= nil do
    print("  [" .. tostring(k) .. "] = " .. tostring(v))
    k, v = next(t2, k)
end
print()

-- Test 9: rawget and rawset
print("Test 9: rawget and rawset")
local t4 = {}
rawset(t4, "key1", "value1")
rawset(t4, 1, "first")
rawset(t4, 2, "second")

print("rawget(t4, 'key1') =", rawget(t4, "key1"))
print("rawget(t4, 1) =", rawget(t4, 1))
print("rawget(t4, 2) =", rawget(t4, 2))
print("rawget(t4, 'nonexistent') =", rawget(t4, "nonexistent"))
print()

-- Test 10: rawlen
print("Test 10: rawlen")
print("rawlen(t1) =", rawlen(t1))
print("rawlen(t3) =", rawlen(t3))
print("rawlen('hello') =", rawlen("hello"))
print()

-- Test 11: rawequal
print("Test 11: rawequal")
local a = {1, 2, 3}
local b = {1, 2, 3}
local c = a
print("rawequal(a, b) =", rawequal(a, b))  -- false, different objects
print("rawequal(a, c) =", rawequal(a, c))  -- true, same object
print("rawequal(42, 42) =", rawequal(42, 42))  -- true
print("rawequal('hello', 'hello') =", rawequal("hello", "hello"))  -- true
print()

-- Test 12: select function
print("Test 12: select function")
print("select('#', 'a', 'b', 'c') =", select("#", "a", "b", "c"))
print("select(2, 'a', 'b', 'c', 'd') =", select(2, "a", "b", "c", "d"))
print("select(-1, 'a', 'b', 'c') =", select(-1, "a", "b", "c"))
print()

-- Test 13: assert function
print("Test 13: assert function")
print("assert(true, 'This should pass') =", assert(true, "This should pass"))
print("assert(42, 'Non-nil value') =", assert(42, "Non-nil value"))
print("assert('hello', 'String value') =", assert("hello", "String value"))

-- Note: assert(false) would terminate the program, so we skip that test
print("Skipping assert(false) test to avoid program termination")
print()

-- Test 14: Metatable operations
print("Test 14: Metatable operations")
local mt = {
    __tostring = function(t)
        return "custom_table"
    end
}

local t5 = {}
print("getmetatable(t5) before =", getmetatable(t5))
setmetatable(t5, mt)
print("getmetatable(t5) after =", getmetatable(t5))
print("tostring(t5) with metatable =", tostring(t5))
print()

print("=== Basic Library Tests Complete ===")
