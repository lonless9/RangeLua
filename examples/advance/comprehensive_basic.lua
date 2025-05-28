-- Test: Comprehensive Basic Functionality
-- Expected output:
-- Hello RangeLua
-- 42
-- 3.14
-- true
-- false
-- nil
-- string
-- number
-- boolean
-- boolean
-- nil
-- Global variable: 100
-- Local variable: 200
-- Modified global: 150
-- Multiple arguments: 1 hello true
-- Description: Comprehensive test covering modules 01_basic, 02_types, 03_variables

-- Module 01_basic: Basic printing
print("Hello RangeLua")

-- Module 02_types: All basic types
print(42)           -- number (integer)
print(3.14)         -- number (float)
print(true)         -- boolean true
print(false)        -- boolean false
print(nil)          -- nil

-- Module 02_types: Type function testing
print(type("hello"))    -- string
print(type(42))         -- number
print(type(true))       -- boolean
print(type(false))      -- boolean
print(type(nil))        -- nil

-- Module 03_variables: Global variables
global_var = 100
print("Global variable:", global_var)

-- Module 03_variables: Local variables
local local_var = 200
print("Local variable:", local_var)

-- Module 03_variables: Variable reassignment
global_var = 150
print("Modified global:", global_var)

-- Module 01_basic: Multiple arguments to print
print("Multiple arguments:", 1, "hello", true)
