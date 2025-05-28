-- Test closure functionality
-- This script tests basic closure creation and upvalue access

-- Simple closure that captures a local variable
function make_counter(start)
    local count = start or 0
    return function()
        count = count + 1
        return count
    end
end

-- Create a counter starting from 10
local counter = make_counter(10)

-- Test the counter
print("First call:", counter())   -- Should print 11
print("Second call:", counter())  -- Should print 12
print("Third call:", counter())   -- Should print 13

-- Create another counter to test independence
local counter2 = make_counter(100)
print("Counter2 first call:", counter2())  -- Should print 101

-- Test that original counter is unaffected
print("Original counter:", counter())  -- Should print 14

-- Test nested closures
function make_adder(x)
    return function(y)
        return function(z)
            return x + y + z
        end
    end
end

local add5 = make_adder(5)
local add5and3 = add5(3)
print("Nested closure result:", add5and3(2))  -- Should print 10 (5+3+2)
