local position = 30


local function fibonacci(n)
    if n == 1 then
        return 0
    elseif n == 2 then
        return 1
    end

    local a, b = 0, 1
    for i = 3, n do
        a, b = b, a + b
    end
    return b
end

local result = fibonacci(position)
print(result)
