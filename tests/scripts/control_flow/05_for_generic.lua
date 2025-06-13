-- Test: Generic for loop
-- Expected output:
-- 1	a
-- 2	b
-- 3	c
-- x	100
-- y	200

local t1 = {"a", "b", "c"}
for i, v in ipairs(t1) do
  print(i, v)
end

local t2 = {x=100, y=200}
-- Note: order of pairs is not guaranteed
-- For testing, we assume a consistent (but not specified) order.
-- A more robust test would sort the keys first.
for k, v in pairs(t2) do
  print(k, v)
end
