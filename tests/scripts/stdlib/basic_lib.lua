-- Test: Standard Library - Basic functions
-- Expected output:
-- Test 1: print function
-- Hello, World!
-- Multiple	arguments	with	tabs
-- 42	true	nil	mixed types
--
--
-- Test 2: type function
-- type(42) =	number
-- type('hello') =	string
-- type(true) =	boolean
-- type(nil) =	nil
-- type({}) =	table
-- type(print) =	function
--
-- Test 3: tostring function
-- tostring(42) =	42
-- tostring(true) =	true
-- tostring(nil) =	nil
-- tostring('hello') =	hello
--
-- Test 4: tonumber function
-- tonumber('42') =	42
-- tonumber('3.14') =	3.14
-- tonumber('0xFF') =	255
-- tonumber('invalid') =	nil
-- tonumber(42) =	42

-- Test 1: print function
print("Test 1: print function")
print("Hello, World!")
print("Multiple", "arguments", "with", "tabs")
print(42, true, nil, "mixed types")
print()
print()

-- Test 2: type function
print("Test 2: type function")
print("type(42) =", type(42))
print("type('hello') =", type("hello"))
print("type(true) =", type(true))
print("type(nil) =", type(nil))
print("type({}) =", type({}))
print("type(print) =", type(print))
print()

-- Test 3: tostring function
print("Test 3: tostring function")
print("tostring(42) =", tostring(42))
print("tostring(true) =", tostring(true))
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
