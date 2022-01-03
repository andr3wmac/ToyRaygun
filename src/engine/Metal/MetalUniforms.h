/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef METAL_UNIFORMS_HEADER_GUARD
#define METAL_UNIFORMS_HEADER_GUARD

#include "platform/Platform.h"

#include <bx/math.h>
#include <simd/simd.h>

namespace toyraygun
{
    struct UniformFloat3
    {
        simd::float3 data;
        
        bx::Vec3 get()
        {
            return bx::Vec3(data.x, data.y, data.z);
        }
        
        void set(bx::Vec3 value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
        }
    };

    struct UniformFloat4x4
    {
        simd::float4x4 data;
        
        void get(float* _mtxOut)
        {
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    // Note the transpose.
                    _mtxOut[(i * 4) + j] = data.columns[j][i];
                }
            }
        }
        
        void set(float* _mtxIn)
        {
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    // Note the transpose.
                    data.columns[j][i] = _mtxIn[(i * 4) + j];
                }
            }
        }
    };
}

#endif // METAL_UNIFORMS_HEADER_GUARD
