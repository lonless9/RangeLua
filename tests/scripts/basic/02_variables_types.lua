-- Test: Variables and Types
-- Expected output:
-- string
-- number
-- boolean
-- nil
-- Global variable:	100
-- Local variable:	200
-- Modified global:	150

print(type("hello"))
print(type(42))
print(type(true))
print(type(nil))

global_var = 100
print("Global variable:", global_var)

local local_var = 200
print("Local variable:", local_var)

global_var = 150
print("Modified global:", global_var)
