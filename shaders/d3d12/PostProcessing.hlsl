/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
*/

#include "shaders/common.h"

RWTexture2D<float4> InputTexture : register(u0);
RWTexture2D<float4> OutputTexture : register(u1);

ConstantBuffer<Uniforms> uniforms : register(b0);

[numthreads(1, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    float3 color = InputTexture[idx.xy].rgb;

    // Apply a very simple tonemapping function to reduce the dynamic range of the
    // input image into a range which can be displayed on screen.
    color = color / (1.0f + color);

    OutputTexture[idx.xy] = float4(color.rgb, 1.0);
}