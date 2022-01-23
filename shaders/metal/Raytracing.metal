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

#define RAY_MASK_PRIMARY   3
#define RAY_MASK_SHADOW    1
#define RAY_MASK_SECONDARY 1

struct Ray
{
    packed_float3 origin;
    uint mask;
    packed_float3 direction;
    float maxDistance;
    float3 color;
};

// Represents an intersection between a ray and the scene, returned by the MPSRayIntersector.
struct Intersection
{
    float distance;
    int primitiveIndex;
    
    // Barycentric coordinates
    float2 coordinates;
};

// Ray Generation
kernel void raygen(uint2 tid [[thread_position_in_grid]],
                  // Buffers bound on the CPU. Note that 'constant' should be used for small
                  // read-only data which will be reused across threads. 'device' should be
                  // used for writable data or data which will only be used by a single thread.
                  constant Uniforms & uniforms,
                  device Ray *rays,
                  texture2d<unsigned int> randomTex,
                  texture2d<float, access::write> dstTex)
{
    // Since we aligned the thread count to the threadgroup size, the thread index may be out of bounds
    // of the render target size.
    if (tid.x < uniforms.width && tid.y < uniforms.height)
    {
        // Compute linear ray index from 2D position
        unsigned int rayIdx = tid.y * uniforms.width + tid.x;

        // Ray we will produce
        device Ray & ray = rays[rayIdx];

        // Pixel coordinates for this thread
        float2 pixel = (float2)tid;

        // Apply a random offset to random number index to decorrelate pixels
        unsigned int offset = randomTex.read(tid).x;
        
        // Add a random offset to the pixel coordinates for antialiasing
        float2 r = float2(halton(offset + uniforms.frameIndex, 0),
                          halton(offset + uniforms.frameIndex, 1));
        
        pixel += r;
        
        // Map pixel coordinates to -1..1
        float2 uv = (float2)pixel / float2(uniforms.width, uniforms.height);
        uv = uv * 2.0f - 1.0f;
        
        // Compute world space position from inverse projection matrix
        constant Camera& camera = uniforms.camera;
        float4 world = float4(uv.xy, 0.0f, 1.0f) * camera.invViewProjMtx;
        world.xyz /= world.w;
        
        // Setup ray.
        ray.origin = camera.position;
        ray.direction = normalize(world.xyz - camera.position);
        ray.mask = RAY_MASK_PRIMARY;
        ray.maxDistance = INFINITY;
        ray.color = float3(1.0f, 1.0f, 1.0f);
        
        // Clear the destination image to black
        dstTex.write(float4(0.0f, 0.0f, 0.0f, 0.0f), tid);
    }
}

// Interpolates vertex attribute of an arbitrary type across the surface of a triangle
// given the barycentric coordinates and triangle index in an intersection struct
template<typename T>
inline T interpolateVertexAttribute(device T *attributes, Intersection intersection)
{
    // Barycentric coordinates sum to one
    float3 uvw;
    uvw.xy = intersection.coordinates;
    uvw.z = 1.0f - uvw.x - uvw.y;
    
    unsigned int triangleIndex = intersection.primitiveIndex;
    
    // Lookup value for each vertex
    T T0 = attributes[triangleIndex * 3 + 0];
    T T1 = attributes[triangleIndex * 3 + 1];
    T T2 = attributes[triangleIndex * 3 + 2];
    
    // Compute sum of vertex attributes weighted by barycentric coordinates
    return uvw.x * T0 + uvw.y * T1 + uvw.z * T2;
}

// Consumes ray/triangle intersection results to compute the shaded image
kernel void primaryHit(uint2 tid [[thread_position_in_grid]],
                        constant Uniforms & uniforms,
                        device Ray *rays,
                        device Ray *shadowRays,
                        device Intersection *intersections,
                        device packed_float3 *vertexColors,
                        device packed_float3 *vertexNormals,
                        device uint *triangleMasks,
                        constant unsigned int & bounce,
                        texture2d<unsigned int> randomTex,
                        texture2d<float, access::write> dstTex)
{
    // Exit if we're not inside the screen.
    if (tid.x >= uniforms.width || tid.y >= uniforms.height)
    {
        return;
    }
    
    unsigned int rayIdx = tid.y * uniforms.width + tid.x;
    device Ray& ray = rays[rayIdx];
    device Ray& shadowRay = shadowRays[rayIdx];
    device Intersection & intersection = intersections[rayIdx];
    
    // Negative distance is a terminated ray.
    if (ray.maxDistance < 0.0f || intersection.distance < 0.0f)
    {
        ray.maxDistance = -1.0f;
        shadowRay.maxDistance = -1.0f;
        return;
    }
    
    float3 color = ray.color;
    uint materialID = triangleMasks[intersection.primitiveIndex];

    // Regular opaque geometry.
    if (materialID == MATERIAL_DEFAULT)
    {
        // Compute intersection point
        float3 intersectionPoint = ray.origin + ray.direction * intersection.distance;

        // Interpolate the vertex color at the intersection point
        float3 vertexColor = interpolateVertexAttribute(vertexColors, intersection);
        
        // Interpolate the vertex normal at the intersection point
        float3 vertexNormal = interpolateVertexAttribute(vertexNormals, intersection);
        vertexNormal = normalize(vertexNormal);

        unsigned int offset = randomTex.read(tid).x;
        
        // Look up two random numbers for this thread
        float2 r = float2(halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 0),
                          halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 1));
        
        LightSample light = sampleAreaLight(uniforms.light, r, intersectionPoint, vertexNormal);
        
        // Add the vertex color to the ray color.
        color *= vertexColor;
        
        // Setup Shadow Ray.
        shadowRay.origin = intersectionPoint + vertexNormal * 1e-3f;
        shadowRay.direction = light.direction;
        shadowRay.mask = RAY_MASK_SHADOW;
        shadowRay.maxDistance = light.distance - 1e-3f;
        shadowRay.color = light.color * color;
            
        // Next we choose a random direction to continue the path of the ray. This will
        // cause light to bounce between surfaces. Normally we would apply a fair bit of math
        // to compute the fraction of reflected by the current intersection point to the
        // previous point from the next point. However, by choosing a random direction with
        // probability proportional to the cosine (dot product) of the angle between the
        // sample direction and surface normal, the math entirely cancels out except for
        // multiplying by the interpolated vertex color. This sampling strategy also reduces
        // the amount of noise in the output image.
        r = float2(halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 2),
                   halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 3));
        
        float3 sampleDirection = sampleCosineWeightedHemisphere(r);
        sampleDirection = alignHemisphereWithNormal(sampleDirection, vertexNormal);

        // Setup ray.
        ray.origin = intersectionPoint + vertexNormal * 1e-3f;
        ray.direction = sampleDirection;
        ray.color = color;
        ray.mask = RAY_MASK_SECONDARY;
    }
    else if (materialID == MATERIAL_EMISSIVE)
    {
        // In this case, a ray coming from the camera hit the light source directly, so
        // we'll write the light color into the output image.
        dstTex.write(float4(uniforms.light.color, 1.0f), tid);
        
        // Terminate the ray's path
        ray.maxDistance = -1.0f;
        shadowRay.maxDistance = -1.0f;
    }
    else
    {
        // Material error, output magenta.
        dstTex.write(float4(1.0, 0.0, 1.0, 1.0f), tid);
    }
}

// Checks if a shadow ray hit something on the way to the light source. If not, the point the
// shadow ray started from was not in shadow so it's color should be added to the output image.
kernel void shadowHit(uint2 tid [[thread_position_in_grid]],
                     constant Uniforms & uniforms,
                     device Ray *shadowRays,
                     device float *intersections,
                     texture2d<float, access::read> srcTex,
                     texture2d<float, access::write> dstTex)
{
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
        unsigned int rayIdx = tid.y * uniforms.width + tid.x;
        device Ray & shadowRay = shadowRays[rayIdx];
        
        // Use the MPSRayIntersection intersectionDataType property to return the
        // intersection distance for this kernel only. You don't need the other fields, so
        // you'll save memory bandwidth.
        float intersectionDistance = intersections[rayIdx];
        
        float3 color = srcTex.read(tid).xyz;
        
        // If the shadow ray wasn't disabled (max distance >= 0) and it didn't hit anything
        // on the way to the light source, add the color passed along with the shadow ray
        // to the output image.
        if (shadowRay.maxDistance >= 0.0f && intersectionDistance < 0.0f)
            color += shadowRay.color;
        
        // Write result to render target
        dstTex.write(float4(color, 1.0f), tid);
    }
}
