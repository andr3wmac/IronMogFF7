local ROOT_DIR = "./"

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
    kind "ConsoleApp"
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
    }

    files { 
        path.join(ROOT_DIR, "src/**.cpp"),
        path.join(ROOT_DIR, "src/**.c"),
        path.join(ROOT_DIR, "src/**.h")
    }