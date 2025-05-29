-- Test cases for RangeLua string library functions
-- This file tests all implemented functions in stdlib::string

print("=== RangeLua String Library Test Suite ===")
print()

-- Test 1: string.byte function
print("Test 1: string.byte function")
print("string.byte('A') =", string.byte('A'))
print("string.byte('Hello', 1) =", string.byte('Hello', 1))
print("string.byte('Hello', 2) =", string.byte('Hello', 2))
print("string.byte('Hello', 1, 3) =", string.byte('Hello', 1, 3))
print("string.byte('Hello', -1) =", string.byte('Hello', -1))  -- Last character
print()

-- Test 2: string.char function
print("Test 2: string.char function")
print("string.char(65) =", string.char(65))  -- 'A'
print("string.char(72, 101, 108, 108, 111) =", string.char(72, 101, 108, 108, 111))  -- 'Hello'
print("string.char(48, 49, 50) =", string.char(48, 49, 50))  -- '012'
print()

-- Test 3: string.len function
print("Test 3: string.len function")
print("string.len('') =", string.len(''))
print("string.len('Hello') =", string.len('Hello'))
print("string.len('Hello, World!') =", string.len('Hello, World!'))
print("string.len('中文') =", string.len('中文'))
print()

-- Test 4: string.lower function
print("Test 4: string.lower function")
print("string.lower('HELLO') =", string.lower('HELLO'))
print("string.lower('Hello World') =", string.lower('Hello World'))
print("string.lower('MiXeD cAsE') =", string.lower('MiXeD cAsE'))
print("string.lower('123ABC') =", string.lower('123ABC'))
print()

-- Test 5: string.upper function
print("Test 5: string.upper function")
print("string.upper('hello') =", string.upper('hello'))
print("string.upper('Hello World') =", string.upper('Hello World'))
print("string.upper('MiXeD cAsE') =", string.upper('MiXeD cAsE'))
print("string.upper('123abc') =", string.upper('123abc'))
print()

-- Test 6: string.reverse function
print("Test 6: string.reverse function")
print("string.reverse('hello') =", string.reverse('hello'))
print("string.reverse('12345') =", string.reverse('12345'))
print("string.reverse('a') =", string.reverse('a'))
print("string.reverse('') =", string.reverse(''))
print()

-- Test 7: string.rep function
print("Test 7: string.rep function")
print("string.rep('a', 5) =", string.rep('a', 5))
print("string.rep('Hello', 3) =", string.rep('Hello', 3))
print("string.rep('x', 0) =", string.rep('x', 0))
print("string.rep('ab', 4) =", string.rep('ab', 4))
print()

-- Test 8: string.sub function
print("Test 8: string.sub function")
local str = "Hello, World!"
print("Original string:", str)
print("string.sub(str, 1, 5) =", string.sub(str, 1, 5))
print("string.sub(str, 8) =", string.sub(str, 8))
print("string.sub(str, -6) =", string.sub(str, -6))
print("string.sub(str, 1, -1) =", string.sub(str, 1, -1))
print("string.sub(str, -6, -2) =", string.sub(str, -6, -2))
print()

-- Test 9: string.find function
print("Test 9: string.find function")
print("string.find('hello world', 'world') =", string.find('hello world', 'world'))
print("string.find('hello world', 'o') =", string.find('hello world', 'o'))
print("string.find('hello world', 'o', 6) =", string.find('hello world', 'o', 6))
print("string.find('hello world', 'xyz') =", string.find('hello world', 'xyz'))
print()

-- Test 10: string.match function
print("Test 10: string.match function")
print("string.match('hello123', '%d+') =", string.match('hello123', '%d+'))
print("string.match('abc123def', '%d+') =", string.match('abc123def', '%d+'))
print("string.match('hello', '%a+') =", string.match('hello', '%a+'))
print("string.match('no numbers', '%d+') =", string.match('no numbers', '%d+'))
print()

-- Test 11: string.gsub function
print("Test 11: string.gsub function")
print("string.gsub('hello world', 'l', 'L') =", string.gsub('hello world', 'l', 'L'))
print("string.gsub('hello world', 'o', 'O') =", string.gsub('hello world', 'o', 'O'))
print("string.gsub('123-456-789', '-', '.') =", string.gsub('123-456-789', '-', '.'))
print()

-- Test 12: string.format function
print("Test 12: string.format function")
print("string.format('Hello %s', 'World') =", string.format('Hello %s', 'World'))
print("string.format('%d + %d = %d', 2, 3, 5) =", string.format('%d + %d = %d', 2, 3, 5))
print("string.format('%.2f', 3.14159) =", string.format('%.2f', 3.14159))
print("string.format('%x', 255) =", string.format('%x', 255))
print()

-- Test 13: String concatenation and mixed operations
print("Test 13: String concatenation and mixed operations")
local s1 = "Hello"
local s2 = "World"
local s3 = s1 .. ", " .. s2 .. "!"
print("Concatenated string:", s3)
print("Length:", string.len(s3))
print("Uppercase:", string.upper(s3))
print("Substring (1,5):", string.sub(s3, 1, 5))
print()

-- Test 14: Edge cases
print("Test 14: Edge cases")
print("Empty string operations:")
print("  string.len('') =", string.len(''))
print("  string.upper('') =", string.upper(''))
print("  string.lower('') =", string.lower(''))
print("  string.reverse('') =", string.reverse(''))

print("Single character operations:")
print("  string.byte('A') =", string.byte('A'))
print("  string.char(65) =", string.char(65))
print("  string.upper('a') =", string.upper('a'))
print("  string.lower('A') =", string.lower('A'))
print()

-- Test 15: String library integration
print("Test 15: String library integration")
local text = "The Quick Brown Fox Jumps Over The Lazy Dog"
print("Original:", text)
print("Lowercase:", string.lower(text))
print("First word:", string.sub(text, 1, string.find(text, ' ') - 1))
print("Replace 'The' with 'A':", string.gsub(text, 'The', 'A'))
print("Length:", string.len(text))
print()

print("=== String Library Tests Complete ===")
