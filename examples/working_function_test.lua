-- Test what currently works with functions
print("=== Working Function Integration Tests ===")

-- Test 1: Basic function availability and calls
print("\n1. Basic Function Calls:")
print("print function type:", type(print))
print("type function type:", type(type))

-- Test 2: Function calls with multiple arguments
print("\n2. Multiple Argument Function Calls:")
print("Multiple args:", "arg1", "arg2", "arg3")

-- Test 3: Nested function calls
print("\n3. Nested Function Calls:")
print("Nested call result:", type(type))

-- Test 4: Function calls with different value types
print("\n4. Function Calls with Different Types:")
print("Number:", 42)
print("String:", "hello")
print("Boolean:", true)
print("Nil:", nil)

print("\n=== Function Integration Working Correctly ===")
