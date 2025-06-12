-- Test cases for pcall and xpcall

-- Expected output:
-- pcall success:
-- true	10	20
-- pcall failure:
-- false	tests/scripts/pcall_tests.lua:20: test error
-- xpcall success:
-- true	hello	world
-- xpcall failure with message handler:
-- false	Error: tests/scripts/pcall_tests.lua:28: test error
-- xpcall failure with failing message handler:
-- false	error in error handling

print("pcall success:")
local success, r1, r2 = pcall(function() return 10, 20 end)
print(success, r1, r2)

print("pcall failure:")
local success, err = pcall(function() error("test error") end)
print(success, err)

print("xpcall success:")
local success, r1, r2 = xpcall(function() return "hello", "world" end, function(err) return "Error: " .. err end)
print(success, r1, r2)

print("xpcall failure with message handler:")
local success, err = xpcall(function() error("test error") end, function(err) return "Error: " .. err end)
print(success, err)

print("xpcall failure with failing message handler:")
local success, err = xpcall(function() error("test error") end, function(err) error("another error") end)
print(success, err)
