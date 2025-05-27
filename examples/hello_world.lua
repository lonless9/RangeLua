-- Simple hello world Lua script for testing RangeLua
print("Hello, World from RangeLua!")

-- Test basic arithmetic
local a = 10
local b = 20
local sum = a + b
print("Sum of", a, "and", b, "is", sum)

-- Test string operations
local greeting = "Hello"
local name = "RangeLua"
local message = greeting .. ", " .. name .. "!"
print(message)

-- Test function definition and call
local function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

print("Factorial of 5 is", factorial(5))

-- Test table operations
local numbers = {1, 2, 3, 4, 5}
print("Numbers table:")
for i, v in ipairs(numbers) do
    print("  [" .. i .. "] = " .. v)
end

-- Return a value
return "Hello World execution completed successfully!"
