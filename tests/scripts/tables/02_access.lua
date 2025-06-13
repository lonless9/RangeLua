-- Test: Table access and modification
-- Expected output:
-- Value of x: 100
-- Modified value of x: 200
-- New value y: 300

local t = {}
t.x = 100
print("Value of x:", t.x)

t["x"] = 200
print("Modified value of x:", t["x"])

t.y = 300
print("New value y:", t.y)
