function liveModProjectLocation()
    return "build/" .. (_ACTION or "generated")
end

function liveModProject(rootDir)
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    characterset "ASCII"
    location (liveModProjectLocation())

    debugdir "./"

    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_DEPRECATE",
        "_USE_MATH_DEFINES",
    }

    includedirs {
        path.join(rootDir, "include/"),
        path.join(rootDir, "src/")
    }

    files {
        path.join(rootDir, "include/**.h"),
        path.join(rootDir, "src/**.cpp"),
        path.join(rootDir, "src/**.c"),
        path.join(rootDir, "src/**.h")
    }

    filter {}
end

function liveModSystemLinks()
    filter "system:windows"
        links {
            "Psapi",
            "User32",
            "Winmm",
        }

    filter {}
end
