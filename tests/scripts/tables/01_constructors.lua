-- Test: Table constructors
-- Expected output:
-- 10
-- hello
-- true

local t1 = {10, 20, 30}
local t2 = {a = "hello", b = "world"}
local t3 = {[1] = true, [2] = false}

print(t1[1])
print(t2.a)
print(t3[1])
