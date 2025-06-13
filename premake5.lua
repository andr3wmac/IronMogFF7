local ROOT_DIR = "./"
local LIB_DIR = "./lib/"

solution "IronMogFF7"
    startproject "IronMogFF7"

    configurations { "Release", "Debug" }
    platforms { "x86_64" }

    filter "platforms:x86_64"
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
        path.join(LIB_DIR, "glfw/include/"),
        path.join(LIB_DIR, "imgui/"),
        path.join(LIB_DIR, "stb/include/"),
    }

    files { 
        path.join(ROOT_DIR, "src/**.cpp"),
        path.join(ROOT_DIR, "src/**.c"),
        path.join(ROOT_DIR, "src/**.h")
    }

    links { 
        "opengl32",
        path.join(LIB_DIR, "glfw/lib/Release/glfw3"),
        path.join(LIB_DIR, "stb/lib/Release/stb"),
    }