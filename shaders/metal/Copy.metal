/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 
 * This shader is based on Metal for Accelerating Ray Tracing from:
   https://developer.apple.com/documentation/metalperformanceshaders/metal_for_accelerating_ray_tracing
 * By Apple
 */

#include <metal_stdlib>

using namespace metal;

// Screen filling quad in normalized device coordinates
constant float2 quadVertices[] = {
    float2(-1, -1),
    float2(-1,  1),
    float2( 1,  1),
    float2(-1, -1),
    float2( 1,  1),
    float2( 1, -1)
};

struct CopyVertexOut {
    float4 position [[position]];
    float2 uv;
};

// Simple vertex shader which passes through NDC quad positions
vertex CopyVertexOut copyVertex(unsigned short vid [[vertex_id]]) {
    float2 position = quadVertices[vid];
    
    CopyVertexOut out;
    
    out.position = float4(position, 0, 1);
    out.uv = position * 0.5f + 0.5f;
    
    return out;
}

// Simple fragment shader which copies a texture and applies a simple tonemapping function
fragment float4 copyFragment(CopyVertexOut in [[stage_in]],
                             texture2d<float> tex)
{
    constexpr sampler sam(min_filter::nearest, mag_filter::nearest, mip_filter::none);
    
    float3 color = tex.sample(sam, in.uv).xyz;
    
    // Apply a very simple tonemapping function to reduce the dynamic range of the
    // input image into a range which can be displayed on screen.
    color = color / (1.0f + color);
    
    return float4(color, 1.0f);
}
