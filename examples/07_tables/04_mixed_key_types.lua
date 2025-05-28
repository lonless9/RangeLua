-- Test 4: Mixed key types
print("=== Test 4: Mixed key types ===")

local t = {}

-- String keys
t["string_key"] = "string value"
t.dot_key = "dot notation value"

-- Number keys
t[1] = "first"
t[42] = "answer"
t[3.14] = "pi"

-- Boolean keys
t[true] = "true value"
t[false] = "false value"

-- Test retrieval
print("String key:", t["string_key"])
print("Dot notation:", t.dot_key)
print("Number key 1:", t[1])
print("Number key 42:", t[42])
print("Number key 3.14:", t[3.14])
print("Boolean true:", t[true])
print("Boolean false:", t[false])

print("Mixed key type tests completed")
