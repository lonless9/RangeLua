-- Test: Basic coroutine operations
-- Expected output:
-- Status before resume: suspended
-- Inside coroutine, part 1
-- Yielded: true	hello
-- Status after yield: suspended
-- Resuming coroutine with: 42
-- Inside coroutine, part 2
-- Coroutine finished with result: done
-- Status after finish: dead

local co = coroutine.create(function(arg)
  print("Inside coroutine, part 1")
  local success, val = coroutine.yield("hello")
  print("Resuming coroutine with:", val)
  print("Inside coroutine, part 2")
  return "done"
end)

print("Status before resume:", coroutine.status(co))
local success, val = coroutine.resume(co, "start")
print("Yielded:", success, val)
print("Status after yield:", coroutine.status(co))

success, val = coroutine.resume(co, 42)
print("Coroutine finished with result:", val)
print("Status after finish:", coroutine.status(co))
