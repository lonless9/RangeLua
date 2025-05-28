-- Test: Generic for loops
-- Expected output:
-- Description: Tests generic for loops with tables
-- Note:
local t = {10, 20, 30}
for i, v in ipairs(t) do
    print(i, v)
end
