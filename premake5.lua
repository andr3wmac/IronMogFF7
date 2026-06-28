local ROOT_DIR = "./"
local LIB_DIR = "./lib/"
local APPFRAME_DIR = "./lib/AppFrame/"
local LIVEMOD_DIR = "./lib/LiveModFF7/"

-- References to other projects premake5.lua scripts.
includeexternal(APPFRAME_DIR)
includeexternal(LIVEMOD_DIR)

solution "IronMogFF7"
    startproject "IronMogFF7"

    configurations { "Release", "Debug" }
    platforms { "x64" }

    filter "platforms:x64"
        architecture "x86_64"

    filter "configurations:Release*"
        defines { "NDEBUG" }
        optimize "Speed"
        symbols "On"

    filter "configurations:Debug*"
        defines { "_DEBUG" }
        optimize "Debug"
        symbols "On"

    filter {}

project "IronMogFF7"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    characterset "ASCII"
    location ("build/" .. _ACTION)

    debugdir "./"

    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_DEPRECATE",
        "_USE_MATH_DEFINES",
    }

    includedirs {
        path.join(ROOT_DIR, "src/"),
        path.join(LIB_DIR, "LiveModFF7/include/"),
        path.join(APPFRAME_DIR, "include/"),
    }

    files { 
        path.join(ROOT_DIR, "src/**.cpp"),
        path.join(ROOT_DIR, "src/**.c"),
        path.join(ROOT_DIR, "src/**.h")
    }

    removefiles {
        path.join(ROOT_DIR, "src/app/gui/GUI.cpp"),
        path.join(ROOT_DIR, "src/app/gui/imgui_impl_glfw.cpp"),
        path.join(ROOT_DIR, "src/app/gui/imgui_impl_glfw.h"),
        path.join(ROOT_DIR, "src/app/gui/imgui_impl_opengl2.cpp"),
        path.join(ROOT_DIR, "src/app/gui/imgui_impl_opengl2.h"),
    }

    links { 
        "AppFrame",
        "LiveModFF7",
    }
    appFrameLinks(APPFRAME_DIR)

    filter {}

group "lib"

project "AppFrame"
    appFrameProject(APPFRAME_DIR)
    location ("build/" .. _ACTION .. "/lib")

project "LiveModFF7"
    liveModProject(LIVEMOD_DIR)
    location ("build/" .. _ACTION .. "/lib")
