-- Test: Closures and upvalues
-- Expected output:
-- 1
-- 2
-- 3
-- 11
-- 12

function make_counter()
  local i = 0
  return function()
    i = i + 1
    return i
  end
end

local c1 = make_counter()
print(c1())
print(c1())
print(c1())

local c2 = make_counter()
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
c2() -- discard
print(c2()) -- Should be 11
print(c2()) -- Should be 12
