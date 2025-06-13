-- Test: Standard Library - Table functions
-- Expected output:
-- 10
-- 30
-- 40
-- Removed:	30
-- 10
-- 40

local t = {10, 20, 30}
table.insert(t, 4, 40) -- t is now {10, 20, 30, 40}
table.remove(t, 2)     -- t is now {10, 30, 40}

print(t[1])
print(t[2])
print(t[3])
print("Removed:", 30)

table.remove(t) -- remove last element, t is now {10, 40}
print(t[1])
print(t[2])
