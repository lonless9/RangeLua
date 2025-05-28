-- Simple closure test
-- Tests basic upvalue capture and access

local x = 42

function test_closure()
    return function()
        return x
    end
end

local closure = test_closure()
print(closure())  -- Should print 42
