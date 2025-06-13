-- Test: Standard Library - String functions
-- Expected output:
-- Length:	11
-- Substring:	World
-- Repeated:	Lua Lua Lua
-- Lower:	hello world
-- Upper:	HELLO WORLD

local s = "Hello World"
print("Length:", string.len(s))
print("Substring:", string.sub(s, 7, 11))
print("Repeated:", string.rep("Lua ", 3))
print("Lower:", string.lower(s))
print("Upper:", string.upper(s))
