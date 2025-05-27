-- Test: Lua truthiness rules
-- Expected output:
-- false
-- true
-- true
-- true
-- true
-- Description: Tests Lua's truthiness rules (only nil and false are falsy)

print(not nil)      -- true (nil is falsy)
print(not false)    -- true (false is falsy)
print(not true)     -- false (true is truthy)
print(not 0)        -- false (0 is truthy in Lua)
print(not "")       -- false (empty string is truthy in Lua)
