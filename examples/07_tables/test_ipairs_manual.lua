-- Test ipairs function manually
print("=== Manual ipairs test ===")

local t = {
    [1] = "first",
    [2] = "second", 
    [3] = "third"
}

-- Call ipairs manually
local iter, state, control = ipairs(t)
print("ipairs returned:")
print("  iter type:", type(iter))
print("  state type:", type(state))
print("  control type:", type(control))

-- Try calling the iterator manually
if iter then
    print("Calling iterator manually:")
    local k1, v1 = iter(state, control)
    print("  First call:", k1, v1)
    
    if k1 then
        local k2, v2 = iter(state, k1)
        print("  Second call:", k2, v2)
        
        if k2 then
            local k3, v3 = iter(state, k2)
            print("  Third call:", k3, v3)
            
            if k3 then
                local k4, v4 = iter(state, k3)
                print("  Fourth call:", k4, v4)
            end
        end
    end
end

print("Manual test completed")
