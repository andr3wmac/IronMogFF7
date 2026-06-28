function appFrameProjectLocation()
    return "build/" .. (_ACTION or "generated")
end

function appFrameProject(rootDir)
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    characterset "ASCII"
    location (appFrameProjectLocation())

    local libDir = path.join(rootDir, "lib/")

    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_DEPRECATE",
        "_USE_MATH_DEFINES",
    }

    includedirs {
        path.join(rootDir, "include/"),
        path.join(rootDir, "src/"),
        path.join(libDir, "glfw/include/"),
        path.join(libDir, "imgui/"),
        path.join(libDir, "stb/include/"),
        path.join(libDir, "nativefiledialog-extended/include/"),
    }

    files {
        path.join(rootDir, "include/**.h"),
        path.join(rootDir, "src/**.cpp"),
        path.join(rootDir, "src/**.c"),
        path.join(rootDir, "src/**.h")
    }
end

function appFrameLinks(rootDir)
    local libDir = path.join(rootDir, "lib/")

    links {
        path.join(libDir, "glfw/lib/Release/glfw3"),
        path.join(libDir, "stb/lib/Release/stb"),
        path.join(libDir, "nativefiledialog-extended/lib/Release/nfd"),
        "opengl32",
        "comctl32",
    }
end
