-- Test: __index and __newindex metamethods
-- Expected output:
-- Getting key: x
-- Value of t.x is 100
-- Getting key: y
-- Value of t.y is default_value
-- Setting key: z to value: 200

local mt = {
  __index = function(tbl, key)
    print("Getting key:", key)
    if key == "y" then
      return "default_value"
    end
  end,
  __newindex = function(tbl, key, value)
    print("Setting key:", key, "to value:", value)
    rawset(tbl, key, value)
  end
}

local t = {x = 100}
setmetatable(t, mt)

print("Value of t.x is", t.x)
print("Value of t.y is", t.y)
t.z = 200
