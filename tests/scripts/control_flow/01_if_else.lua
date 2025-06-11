-- Test: If/Elseif/Else statements
-- Expected output:
-- x is positive
-- y is zero
-- z is negative
-- a is positive and greater than 5

local x = 10
if x > 0 then
    print("x is positive")
elseif x == 0 then
    print("x is zero")
else
    print("x is negative")
end

local y = 0
if y > 0 then
    print("y is positive")
elseif y == 0 then
    print("y is zero")
else
    print("y is negative")
end

local z = -5
if z > 0 then
    print("z is positive")
else
    print("z is negative")
end

local a = 8
if a > 0 then
    if a > 5 then
        print("a is positive and greater than 5")
    else
        print("a is positive but not greater than 5")
    end
end
