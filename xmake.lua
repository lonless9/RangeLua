-- Project configuration
set_project("RangeLua")
set_version("0.1.0")
set_languages("c++20")
set_toolchains("clang")

-- Add build modes
add_rules("mode.debug", "mode.release")

add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode", lsp = "clangd"})

-- C++20 specific flags
add_cxxflags("-std=c++20", "-fcoroutines")

-- Debug configuration
if is_mode("debug") then
    add_cxxflags("-fsanitize=address", "-fno-omit-frame-pointer", "-g", "-O0")
    add_ldflags("-fsanitize=address")
    add_defines("RANGELUA_DEBUG", "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
else
    -- Release configuration without LTO to avoid fmt library global initialization issues
    add_cxxflags("-O3", "-DNDEBUG")
    add_defines("SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO")
    -- Note: LTO disabled due to fmt library global variable initialization conflicts
end

-- Compiler warnings and standards
add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Werror")
add_cxxflags("-Wno-unused-parameter", "-Wno-missing-field-initializers")

-- Enable compile commands for IDE support
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

-- Set build directory
set_targetdir("build/$(plat)_$(arch)_$(mode)")

-- Package dependencies
add_requires("spdlog")
add_requires("catch2", {version = "3.x"})

-- Core library target
target("rangelua_core")
    set_kind("static")
    add_packages("spdlog")

    -- Include directories
    add_includedirs("include", {public = true})
    add_includedirs("src", {private = true})

    -- Source files
    add_files("src/core/**.cpp")
    add_files("src/frontend/**.cpp")
    add_files("src/backend/**.cpp")
    add_files("src/runtime/**.cpp")
    add_files("src/api/**.cpp")
    add_files("src/stdlib/**.cpp")
    add_files("src/utils/**.cpp")

    -- Header files for IDE
    add_headerfiles("include/rangelua/**.hpp")

-- Main interpreter target
target("rangelua")
    set_kind("binary")
    add_packages("spdlog")
    set_rundir("$(projectdir)")
    add_deps("rangelua_core")
    add_files("src/main.cpp")

-- Test runner target
target("tests")
    set_kind("binary")
    add_packages("spdlog", "catch2")
    set_rundir("$(projectdir)") -- So it can find the tests/scripts directory
    add_deps("rangelua_core")
    add_files("tests/main.cpp", "tests/integration_tests.cpp")
    if is_plat("linux", "macosx") then
        add_syslinks("pthread", "dl")
    end
