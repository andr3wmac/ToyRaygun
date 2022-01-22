/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE

 * This shader is based on D3D12 Raytracing Sample from:
     https://github.com/microsoft/DirectX-Graphics-Samples/
 * By Microsoft
*/

#include "shaders/common.h"

#define MAX_BOUNCES 3

struct Vertex
{
    float3 position;
    float3 normal;
    float3 color;
};

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);
//RWTexture2D<float4> RandomTexture : register(u1);
ByteAddressBuffer Indices : register(t2, space0);
StructuredBuffer<Vertex> Vertices : register(t3, space0);
StructuredBuffer<uint> MaterialIDs : register(t4, space0);

ConstantBuffer<Uniforms> g_sceneCB : register(b0);

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(uint offsetBytes)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);

    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload
{
    float4 color;
    uint recursionDepth;
};

struct ShadowPayload
{
    bool hit;
};

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index;

    // Apply a random offset to random number index to decorrelate pixels
    uint randSeed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, g_sceneCB.frameIndex, 16);
    float2 rndFloat2 = float2(nextRand(randSeed), nextRand(randSeed));

    xy += rndFloat2; // Add a random offset to the pixel coordinates for antialiasing
    xy += 0.5f; // center in the middle of the pixel.

    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), g_sceneCB.camera.invViewProjMtx);

    world.xyz /= world.w;
    origin = g_sceneCB.camera.position.xyz;
    direction = normalize(world.xyz - origin);
}

float4 tracePrimaryRay(RayDesc ray, uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_BOUNCES)
    {
        return float4(0, 0, 0, 0);
    }

    RayPayload payload = { float4(0, 0, 0, 0), currentRayRecursionDepth + 1 };

    TraceRay(Scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,    // RayFlags
        0xFF,                                   // InstanceInclusionMask
        0,                                      // RayContributionToHitGroupIndex
        1,                                      // MultiplierForGeometryContributionToHitGroupIndex
        0,                                      // MissShaderIndex
        ray,
        payload);

    return payload.color;
}

bool traceShadowRay(RayDesc ray, uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_BOUNCES)
    {
        return true;
    }

    ShadowPayload shadowPayload;
    shadowPayload.hit = false;

    TraceRay(Scene,
        0,              // RayFlags
        0xFF,           // InstanceInclusionMask
        1,              // RayContributionToHitGroupIndex
        0,              // MultiplierForGeometryContributionToHitGroupIndex
        1,              // MissShaderIndex
        ray,
        shadowPayload
    );

    return shadowPayload.hit;
}

[shader("raygeneration")]
void raygen()
{
    float3 rayDir;
    float3 origin;

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    uint currentRecursionDepth = 0;
    float4 color = tracePrimaryRay(ray, currentRecursionDepth);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = color;
}

[shader("closesthit")]
void primaryHit(inout RayPayload payload, in MyAttributes attr)
{
    float3 hitPosition = HitWorldPosition();

    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint triangleIndex = PrimitiveIndex();
    uint baseIndex = triangleIndex * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 vertexNormals[3] = {
        Vertices[indices[0]].normal,
        Vertices[indices[1]].normal,
        Vertices[indices[2]].normal
    };
    float3 vertexNormal = HitAttribute(vertexNormals, attr);

    // Retrieve corresponding vertex colors for the triangle vertices.
    float3 vertexColors[3] = {
        Vertices[indices[0]].color,
        Vertices[indices[1]].color,
        Vertices[indices[2]].color
    };
    float3 vertexColor = HitAttribute(vertexColors, attr);

    uint materialID = MaterialIDs[triangleIndex];

    // Error
    payload.color = float4(1.0, 0.0, 1.0, 1.0);

    // Default
    if (materialID == MATERIAL_DEFAULT)
    {
        // Initialize a random number generator
        uint randSeed = initRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, g_sceneCB.frameIndex, 16);

        // Get random numbers (in polar coordinates), convert to random cartesian uv on the lens
        float2 rndFloat2 = float2(nextRand(randSeed), nextRand(randSeed));
        float2 rnd = float2(2.0f * 3.14159265f * rndFloat2.x, rndFloat2.y);
        float2 rndUV = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);

        // Apply a random offset to random number index to decorrelate pixels
        //uint offset = (uint)RandomTexture.Load(DispatchRaysIndex().xy).x;
        uint offset = randSeed;
        float2 r = float2(halton(offset + g_sceneCB.frameIndex, 0), halton(offset + g_sceneCB.frameIndex, 1));

        LightSample light;
        light = sampleAreaLight(g_sceneCB.light, r, hitPosition, vertexNormal);

        float3 color = light.color * vertexColor;

        // Trace Shadow Ray
        RayDesc shadowRay;
        shadowRay.Origin = hitPosition;
        shadowRay.Direction = light.direction;
        shadowRay.TMin = 0.001;
        shadowRay.TMax = 10000.0;
        bool shadowRayHit = traceShadowRay(shadowRay, payload.recursionDepth);
        float shadowFactor = shadowRayHit ? 0.0 : 1.0;

        float3 sampleDirection = sampleCosineWeightedHemisphere(rndUV);
        sampleDirection = alignHemisphereWithNormal(sampleDirection, vertexNormal);
        sampleDirection = normalize(sampleDirection);

        RayDesc secondaryRay;
        secondaryRay.Origin = hitPosition;
        secondaryRay.Direction = normalize(sampleDirection);
        secondaryRay.TMin = 0.001;
        secondaryRay.TMax = 10000.0;

        float4 secondaryColor = tracePrimaryRay(secondaryRay, payload.recursionDepth);

        // Final Color
        payload.color = (float4(color.rgb, 1.0f) * shadowFactor) + secondaryColor;
    }

    // Emissive
    if (materialID == MATERIAL_EMISSIVE)
    {
        payload.color = float4(vertexColor, 1.0);
    }
}

[shader("miss")]
void primaryMiss(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.0f, 0.0f, 1.0f);
    payload.color = background;
}

[shader("closesthit")]
void shadowHit(inout ShadowPayload payload, in MyAttributes attribs)
{
    payload.hit = true;

    uint triangleIndex = PrimitiveIndex();
    uint materialID = MaterialIDs[triangleIndex];

    // Emissive
    if (materialID == MATERIAL_EMISSIVE)
    {
        // Hitting a light means we're not in shadow.
        payload.hit = false;
    }
}

[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}