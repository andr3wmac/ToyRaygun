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

    sysincludedirs {
        path.join(LIB_DIR, "bx/include/")  
    }

    includedirs {
        SRC_DIR
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

    filter "configurations:Debug*"
        defines {
            "BX_CONFIG_DEBUG=1",
        }

    filter "configurations:Release*"
        defines {
            "BX_CONFIG_DEBUG=0",
        }

    filter "system:windows"
        includedirs {
            path.join(LIB_DIR, "SDL2-2.0.18/include/"),  
            path.join(LIB_DIR, "bx/include/compat/msvc/"),
            path.join(LIB_DIR, "DirectXShaderCompiler/inc/")  
        }

        files {
            path.join(SRC_DIR, "Renderer/D3D12/**.cpp"),
            path.join(SRC_DIR, "Renderer/D3D12/**.h")
        }

        links { 
            "d3d12",
            "dxgi",
            "dxguid",
            "d3dcompiler",

            path.join(LIB_DIR, "SDL2-2.0.18/lib/x64/SDL2"),
            path.join(LIB_DIR, "bx/lib/x64/bxRelease"),
            path.join(LIB_DIR, "DirectXShaderCompiler/lib/x64/dxcompiler")              
        }

        postbuildcommands {
            "{COPYFILE} \"" .. path.getabsolute(path.join(LIB_DIR, "SDL2-2.0.18/lib/x64/SDL2.dll")) .. "\" \"%{cfg.buildtarget.directory}/SDL2.dll\"",
            "{COPYFILE} \"" .. path.getabsolute(path.join(LIB_DIR, "DirectXShaderCompiler/bin/x64/dxcompiler.dll")) .. "\" \"%{cfg.buildtarget.directory}/dxcompiler.dll\"",
        }

    filter "system:macosx"
        files {
            path.join(SRC_DIR, "Renderer/Metal/**.mm"),
            path.join(SRC_DIR, "Renderer/Metal/**.h"),
            path.join(SRC_DIR, "shaders/**.metal")
        }

        libdirs { 
            path.join(LIB_DIR, "bx/lib/osx/")
        }

        links { 
            "bxRelease",

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