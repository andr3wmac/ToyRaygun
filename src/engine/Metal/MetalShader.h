/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef METAL_SHADER_HEADER_GUARD
#define METAL_SHADER_HEADER_GUARD

#include "engine/Shader.h"

namespace toyraygun
{
    class MetalShader : public Shader
    {
    public:
        virtual bool compile();
    };
}

#endif // METAL_SHADER_HEADER_GUARD