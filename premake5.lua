local pkgconfig = require 'pkgconfig'

local LIB_DIR = "lib/"
local SRC_DIR = "src/"

solution "ToyRaygun"
    startproject "ToyRaygun"

    configurations { "Release", "Debug" }
    architecture "ARM64"
    systemversion "11.0.0"

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
    cppdialect "C++14"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    characterset "ASCII"
    location ("build/" .. _ACTION)

    debugdir "./runtime/"

    defines {
        "_THREAD_SAFE"
    }

    files { 
        path.join(SRC_DIR, "**.cpp"),
        path.join(SRC_DIR, "**.h"),
    }

    pkgconfig.add_includes("sdl2")
    pkgconfig.add_links("sdl2")

    filter "system:macosx"
        files {
            path.join(SRC_DIR, "**.mm"),
            path.join(SRC_DIR, "shaders/**.metal")
        }

        links { 
            "Metal.framework",
            "MetalPerformanceShaders.framework",
            "QuartzCore.framework",     
        }

    filter {}