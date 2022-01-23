/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
*/

#include "shaders/common.h"

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);
ConstantBuffer<Uniforms> g_uniforms : register(b0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput vert(float4 position : POSITION, float2 uv : TEXCOORD0)
{
    PSInput result;

    result.position = position;
    result.uv = uv;

    return result;
}

float4 frag(PSInput input) : SV_TARGET
{
    float3 color = g_texture.Sample(g_sampler, input.uv).rgb;

    // Tonemapping.
    color = ACESFilm(color);

    // Convert to SRGB.
    color = float3(D3DX_FLOAT_to_SRGB(color.r), D3DX_FLOAT_to_SRGB(color.g), D3DX_FLOAT_to_SRGB(color.b));

    return float4(color.rgb, 1.0);
}