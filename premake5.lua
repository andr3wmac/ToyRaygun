local LIB_DIR = "lib/"
local SRC_DIR = "src/"

solution "ToyRaygun"
    startproject "ToyRaygun"

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
        
project "ToyRaygun"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    characterset "ASCII"
    location ("build/" .. _ACTION)

    debugdir "./runtime/"

    files { 
        path.join(SRC_DIR, "**.cpp"),
        path.join(SRC_DIR, "**.h")
    }

    includedirs {
        path.join(LIB_DIR, "SDL2-2.0.18/include/"),  
    }

    links { 
        "d3d12",
        "dxgi",
        "d3dcompiler",

        path.join(LIB_DIR, "SDL2-2.0.18/lib/x64/SDL2"),         
    }

    postbuildcommands {
        "{COPYFILE} \"" .. path.getabsolute(path.join(LIB_DIR, "SDL2-2.0.18/lib/x64/SDL2.dll")) .. "\" \"%{cfg.buildtarget.directory}/SDL2.dll\"",
    }

    filter {}