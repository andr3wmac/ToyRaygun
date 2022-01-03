/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Metal shaders used for ray tracing
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

// Generates rays starting from the camera origin and traveling towards the image plane aligned
// with the camera's coordinate system.
kernel void rayKernel(uint2 tid [[thread_position_in_grid]],
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
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
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
        
        // Rays start at the camera position
        constant Camera& camera = uniforms.camera;
        ray.origin = camera.position;
        
        // Compute world space position from inverse projection matrix
        float4 world = float4(uv.xy, 0.0f, 1.0f) * camera.invViewProjMtx;
        world.xyz /= world.w;
        
        // Use that for ray direction.
        ray.direction = normalize(world.xyz - camera.position);
        
        // The camera emits primary rays
        ray.mask = RAY_MASK_PRIMARY;
        
        // Don't limit intersection distance
        ray.maxDistance = INFINITY;
        
        // Start with a fully white color. Each bounce will scale the color as light
        // is absorbed into surfaces.
        ray.color = float3(1.0f, 1.0f, 1.0f);
        
        // Clear the destination image to black
        dstTex.write(float4(0.0f, 0.0f, 0.0f, 0.0f), tid);
    }
}

// Interpolates vertex attribute of an arbitrary type across the surface of a triangle
// given the barycentric coordinates and triangle index in an intersection struct
template<typename T>
inline T interpolateVertexAttribute(device T *attributes, Intersection intersection) {
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

// Uses the inversion method to map two uniformly random numbers to a three dimensional
// unit hemisphere where the probability of a given sample is proportional to the cosine
// of the angle between the sample direction and the "up" direction (0, 1, 0)
inline float3 sampleCosineWeightedHemisphere(float2 u) {
    float phi = 2.0f * M_PI_F * u.x;
    
    float cos_phi;
    float sin_phi = sincos(phi, cos_phi);
    
    float cos_theta = sqrt(u.y);
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    
    return float3(sin_theta * cos_phi, cos_theta, sin_theta * sin_phi);
}

// Maps two uniformly random numbers to the surface of a two-dimensional area light
// source and returns the direction to this point, the amount of light which travels
// between the intersection point and the sample point on the light source, as well
// as the distance between these two points.
inline void sampleAreaLight(constant AreaLight & light,
                            float2 u,
                            float3 position,
                            thread float3 & lightDirection,
                            thread float3 & lightColor,
                            thread float & lightDistance)
{
    // Map to -1..1
    u = u * 2.0f - 1.0f;
    
    // Transform into light's coordinate system
    float3 samplePosition = light.position +
                            light.right * u.x +
                            light.up * u.y;
    
    // Compute vector from sample point on light source to intersection point
    lightDirection = samplePosition - position;
    
    lightDistance = length(lightDirection);
    
    float inverseLightDistance = 1.0f / max(lightDistance, 1e-3f);
    
    // Normalize the light direction
    lightDirection *= inverseLightDistance;
    
    // Start with the light's color
    lightColor = light.color;
    
    // Light falls off with the inverse square of the distance to the intersection point
    lightColor *= (inverseLightDistance * inverseLightDistance);
    
    // Light also falls off with the cosine of angle between the intersection point and
    // the light source
    lightColor *= saturate(dot(-lightDirection, light.forward));
}

// Aligns a direction on the unit hemisphere such that the hemisphere's "up" direction
// (0, 1, 0) maps to the given surface normal direction
inline float3 alignHemisphereWithNormal(float3 sample, float3 normal) {
    // Set the "up" vector to the normal
    float3 up = normal;
    
    // Find an arbitrary direction perpendicular to the normal. This will become the
    // "right" vector.
    float3 right = normalize(cross(normal, float3(0.0072f, 1.0f, 0.0034f)));
    
    // Find a third vector perpendicular to the previous two. This will be the
    // "forward" vector.
    float3 forward = cross(right, up);
    
    // Map the direction on the unit hemisphere to the coordinate system aligned
    // with the normal.
    return sample.x * right + sample.y * up + sample.z * forward;
}

// Consumes ray/triangle intersection results to compute the shaded image
kernel void shadeKernel(uint2 tid [[thread_position_in_grid]],
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
    if (tid.x < uniforms.width && tid.y < uniforms.height) {
        unsigned int rayIdx = tid.y * uniforms.width + tid.x;
        device Ray & ray = rays[rayIdx];
        device Ray & shadowRay = shadowRays[rayIdx];
        device Intersection & intersection = intersections[rayIdx];
        
        float3 color = ray.color;
        
        // Intersection distance will be negative if ray missed or was disabled in a previous
        // iteration.
        if (ray.maxDistance >= 0.0f && intersection.distance >= 0.0f) {
            uint mask = triangleMasks[intersection.primitiveIndex];

            // The light source is included in the acceleration structure so we can see it in the
            // final image. However, we will compute and sample the lighting directly, so we mask
            // the light out for shadow and secondary rays.
            if (mask == MATERIAL_DEFAULT) {
                // Compute intersection point
                float3 intersectionPoint = ray.origin + ray.direction * intersection.distance;

                // Interpolate the vertex normal at the intersection point
                float3 surfaceNormal = interpolateVertexAttribute(vertexNormals, intersection);
                surfaceNormal = normalize(surfaceNormal);

                unsigned int offset = randomTex.read(tid).x;
                
                // Look up two random numbers for this thread
                float2 r = float2(halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 0),
                                  halton(offset + uniforms.frameIndex, 2 + bounce * 4 + 1));
                
                float3 lightDirection;
                float3 lightColor;
                float lightDistance;
                
                // Compute the direction to, color, and distance to a random point on the light
                // source
                sampleAreaLight(uniforms.light, r, intersectionPoint, lightDirection,
                                lightColor, lightDistance);
                
                // Scale the light color by the cosine of the angle between the light direction and
                // surface normal
                lightColor *= saturate(dot(surfaceNormal, lightDirection));

                // Interpolate the vertex color at the intersection point
                color *= interpolateVertexAttribute(vertexColors, intersection);
                
                // Compute the shadow ray. The shadow ray will check if the sample position on the
                // light source is actually visible from the intersection point we are shading.
                // If it is, the lighting contribution we just computed will be added to the
                // output image.
                
                // Add a small offset to the intersection point to avoid intersecting the same
                // triangle again.
                shadowRay.origin = intersectionPoint + surfaceNormal * 1e-3f;
                
                // Travel towards the light source
                shadowRay.direction = lightDirection;
                
                // Avoid intersecting the light source itself
                shadowRay.mask = RAY_MASK_SHADOW;
                
                // Don't overshoot the light source
                shadowRay.maxDistance = lightDistance - 1e-3f;
                
                // Multiply the color and lighting amount at the intersection point to get the final
                // color, and pass it along with the shadow ray so that it can be added to the
                // output image if needed.
                shadowRay.color = lightColor * color;
                
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
                sampleDirection = alignHemisphereWithNormal(sampleDirection, surfaceNormal);

                //ray.maxDistance = -1.0f;
                //shadowRay.maxDistance = -1.0f;
                ray.origin = intersectionPoint + surfaceNormal * 1e-3f;
                ray.direction = sampleDirection;
                ray.color = color;
                ray.mask = RAY_MASK_SECONDARY;
            }
            else {
                // In this case, a ray coming from the camera hit the light source directly, so
                // we'll write the light color into the output image.
                dstTex.write(float4(uniforms.light.color, 1.0f), tid);
                
                // Terminate the ray's path
                ray.maxDistance = -1.0f;
                shadowRay.maxDistance = -1.0f;
            }
        }
        else {
            // The ray missed the scene, so terminate the ray's path
            ray.maxDistance = -1.0f;
            shadowRay.maxDistance = -1.0f;
        }
    }
}

// Checks if a shadow ray hit something on the way to the light source. If not, the point the
// shadow ray started from was not in shadow so it's color should be added to the output image.
kernel void shadowKernel(uint2 tid [[thread_position_in_grid]],
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