-- Test various Lua constructs that the parser should handle

-- Local variable declarations
local x = 42
local y, z = 1, 2

-- Function declarations
function add(a, b)
    return a + b
end

local function multiply(a, b)
    return a * b
end

-- If statements
if x > 0 then
    print("positive")
elseif x < 0 then
    print("negative")
else
    print("zero")
end

-- While loop
local i = 0
while i < 5 do
    print(i)
    i = i + 1
end

-- For loops
for j = 1, 10 do
    print(j)
end

for k, v in pairs({1, 2, 3}) do
    print(k, v)
end

-- Repeat-until loop
repeat
    i = i - 1
until i <= 0

-- Do block
do
    local temp = 100
    print(temp)
end

-- Table constructor
local table = {
    name = "test",
    [1] = "first",
    "second",
    third = function() return 3 end
}

-- Function expressions
local func = function(x, y, ...)
    return x + y
end

-- Method calls
table:method(1, 2, 3)

-- Table access
print(table.name)
print(table[1])

-- Expressions
local result = (x + y) * z - func(1, 2)
