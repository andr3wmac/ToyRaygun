/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
*/

#include "shaders/common.h"

RWTexture2D<float4> InputTexture : register(u0);
RWTexture2D<float4> OutputTexture : register(u1);

ConstantBuffer<Uniforms> uniforms : register(b0);

[numthreads(1, 1, 1)]
void postProcess(uint3 idx : SV_DispatchThreadID)
{
    float3 color = InputTexture[idx.xy].rgb;

    // Tonemapping.
    color = ACESFilm(color);

    // Convert to SRGB.
    color = float3(D3DX_FLOAT_to_SRGB(color.r), D3DX_FLOAT_to_SRGB(color.g), D3DX_FLOAT_to_SRGB(color.b));

    OutputTexture[idx.xy] = float4(color.rgb, 1.0);
}