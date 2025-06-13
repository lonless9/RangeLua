-- Test: Repeat-until loop
-- Expected output:
-- 1
-- 2
-- 3
-- 4
-- 5
-- Repeat loop finished

local i = 1
repeat
  print(i)
  i = i + 1
until i > 5
print("Repeat loop finished")
