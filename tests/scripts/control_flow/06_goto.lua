-- Test: Goto statement
-- Expected output:
-- Start
-- In loop: 1
-- In loop: 2
-- In loop: 3
-- Jumped to label
-- End

print("Start")
local i = 0
::loop::
i = i + 1
print("In loop:", i)
if i < 3 then
  goto loop
end

goto my_label

print("This should be skipped")

::my_label::
print("Jumped to label")
print("End")
