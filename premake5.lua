local pkgconfig = require 'pkgconfig'

local LIB_DIR = "lib/"
local SRC_DIR = "src/"

solution "ToyRaygun"
    startproject "ToyRaygun"

    configurations { "Release", "Debug" }

    filter "system:windows"
        architecture "x86_64"

    filter "system:macosx"
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
        path.join(SRC_DIR, "*.cpp"), 
        path.join(SRC_DIR, "*.h"),
        path.join(SRC_DIR, "Platform/*.cpp"), 
        path.join(SRC_DIR, "Platform/*.h"),
        path.join(SRC_DIR, "Renderer/*.cpp"), 
        path.join(SRC_DIR, "Renderer/*.h"),
        path.join(SRC_DIR, "Scene/*.cpp"), 
        path.join(SRC_DIR, "Scene/*.h"),
    }

    filter "system:windows"
        includedirs {
            path.join(LIB_DIR, "SDL2-2.0.18/include/"),  
        }

        files {
            path.join(SRC_DIR, "Renderer/D3D12/**.cpp"),
            path.join(SRC_DIR, "Renderer/D3D12/**.h")
        }

        links { 
            "d3d12",
            "dxgi",
            "d3dcompiler",

            path.join(LIB_DIR, "SDL2-2.0.18/lib/x64/SDL2"),         
        }

    filter "system:macosx"
        files {
            path.join(SRC_DIR, "Renderer/Metal/**.mm"),
            path.join(SRC_DIR, "Renderer/Metal/**.h"),
            path.join(SRC_DIR, "shaders/**.metal")
        }

        links { 
            "Metal.framework",
            "MetalPerformanceShaders.framework",
            "QuartzCore.framework",     
        }

    -- use if statement so these functions won't be called at all on windows.
    if os.host() == "macosx" then
        pkgconfig.add_includes("sdl2")
        pkgconfig.add_links("sdl2")
    end

    filter {}