-- Test: Vararg functions
-- Expected output:
-- 1	2	3
-- Total: 9
-- One arg:	a
-- No args.

function print_all(...)
  print(...)
end

function sum(...)
  local total = 0
  for _, v in ipairs({...}) do
    total = total + v
  end
  return total
end

function first_arg(...)
    local arg = {...}
    if #arg > 0 then
        print("One arg:", arg[1])
    else
        print("No args.")
    end
end

print_all(1, 2, 3)
print("Total:", sum(1, 2, 3, 3))
first_arg("a", "b", "c")
first_arg()
