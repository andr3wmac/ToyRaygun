/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 
 * This shader is based on Metal for Accelerating Ray Tracing from:
   https://developer.apple.com/documentation/metalperformanceshaders/metal_for_accelerating_ray_tracing
 * By Apple
 */

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#include "shaders/common.h"

// Accumulates the current frame's image with a running average of all previous frames to
// reduce noise over time.
kernel void accumulateKernel(uint2 tid [[thread_position_in_grid]],
                             constant Uniforms & uniforms,
                             texture2d<float> renderTex,
                             texture2d<float> prevTex,
                             texture2d<float, access::write> accumTex)
{
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
        float3 color = renderTex.read(tid).xyz;

        // Compute the average of all frames including the current frame
        if (uniforms.frameIndex > 0) {
            float3 prevColor = prevTex.read(tid).xyz;
            prevColor *= uniforms.frameIndex;
            
            color += prevColor;
            color /= (uniforms.frameIndex + 1);
        }
        
        accumTex.write(float4(color, 1.0f), tid);
    }
}
