/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
*/

#include "shaders/common.h"

RWTexture2D<float4> InputTexture : register(u0);
RWTexture2D<float4> AccumulateTexture : register(u1);

ConstantBuffer<Uniforms> uniforms : register(b0);

[numthreads(1, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    float3 color = InputTexture[idx.xy].rgb;

    // Compute the average of all frames including the current frame
    if (uniforms.frameIndex > 0)
    {
        float3 prevColor = AccumulateTexture[idx.xy].rgb;
        prevColor *= uniforms.frameIndex;

        color += prevColor;
        color /= (uniforms.frameIndex + 1);
    }

	AccumulateTexture[idx.xy] = float4(color, 1.0f);
}