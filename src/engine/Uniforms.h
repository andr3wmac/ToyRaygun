/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef UNIFORMS_HEADER_GUARD
#define UNIFORMS_HEADER_GUARD

#include "platform/Platform.h"

#ifdef PLATFORM_WINDOWS
#include "engine/D3D12/D3D12Uniforms.h"
#else
#include "engine/Metal/MetalUniforms.h"
#endif

namespace toyraygun
{
    struct Camera {
        UniformFloat3 position;
        UniformFloat4x4 invViewProjMtx;
    };

    struct AreaLight {
        UniformFloat3 position;
        UniformFloat3 forward;
        UniformFloat3 right;
        UniformFloat3 up;
        UniformFloat3 color;
    };

    struct Uniforms
    {
        unsigned int width;
        unsigned int height;
        unsigned int frameIndex;
        Camera camera;
        AreaLight light;
    };
}

#endif // UNIFORMS_HEADER_GUARD
