-- Test: Arithmetic metamethods
-- Expected output:
-- Adding Vector(1, 2) and Vector(3, 4)
-- Subtraction not supported
-- Result: Vector(4, 6)

Vector = {}
function Vector:new(x, y)
  local obj = {x=x, y=y}
  setmetatable(obj, {__add = Vector.add, __tostring = Vector.tostring})
  return obj
end

function Vector.add(v1, v2)
  print("Adding Vector(" .. v1.x .. ", " .. v1.y .. ") and Vector(" .. v2.x .. ", " .. v2.y .. ")")
  return Vector:new(v1.x + v2.x, v1.y + v2.y)
end

function Vector.tostring(v)
    return "Vector(" .. v.x .. ", " .. v.y .. ")"
end

local v1 = Vector:new(1, 2)
local v2 = Vector:new(3, 4)
local v3 = v1 + v2
print("Result:", tostring(v3))

local s, msg = pcall(function() return v1 - v2 end)
if not s then
    print("Subtraction not supported")
end
