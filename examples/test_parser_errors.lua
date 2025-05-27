-- Test file with syntax errors

local x = 42
local y = -- missing value
function test(
    -- missing closing parenthesis and body

if x > 0 then
    print("positive")
-- missing end

for i = 1, 10
    -- missing do
    print(i)
end
