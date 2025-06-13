-- Test: Basic string operations
-- Expected output:
-- hello world
-- 11
-- hellohellohello

local s1 = "hello"
local s2 = " world"

local s3 = s1 .. s2
print(s3)
print(#s3)

print(s1 .. s1 .. s1)
