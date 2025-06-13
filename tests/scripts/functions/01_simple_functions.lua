-- Test: Simple function declaration and calls
-- Expected output:
-- add(5, 3) = 8
-- 10	20

function add(a, b)
  return a + b
end

print("add(5, 3) =", add(5, 3))

function multi_return()
  return 10, 20
end

local x, y = multi_return()
print(x, y)
