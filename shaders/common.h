/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef SHADERS_COMMON_HEADER_GUARD
#define SHADERS_COMMON_HEADER_GUARD

#define MATERIAL_DEFAULT   1
#define MATERIAL_EMISSIVE  2

struct Camera {
    float3 position;
    float4x4 invViewProjMtx;
};

struct AreaLight {
    float3 position;
    float3 forward;
    float3 right;
    float3 up;
    float3 color;
};

struct Uniforms
{
    unsigned int width;
    unsigned int height;
    unsigned int frameIndex;
    Camera camera;
    AreaLight light;
};

// Returns the i'th element of the Halton sequence using the d'th prime number as a
// base. The Halton sequence is a "low discrepency" sequence: the values appear
// random but are more evenly distributed then a purely random sequence. Each random
// value used to render the image should use a different independent dimension 'd',
// and each sample (frame) should use a different index 'i'. To decorrelate each
// pixel, a random offset can be applied to 'i'.
float halton(unsigned int i, unsigned int d)
{
    const unsigned int primes[] =
    {
        2,   3,  5,  7,
        11, 13, 17, 19,
        23, 29, 31, 37,
        41, 43, 47, 53,
    };
    
    unsigned int b = primes[d];
    
    float f = 1.0f;
    float invB = 1.0f / b;
    
    float r = 0;
    
    while (i > 0) {
        f = f * invB;
        r = r + f * (i % b);
        i = i / b;
    }
    
    return r;
}

// Uses the inversion method to map two uniformly random numbers to a three dimensional
// unit hemisphere where the probability of a given sample is proportional to the cosine
// of the angle between the sample direction and the "up" direction (0, 1, 0)
inline float3 sampleCosineWeightedHemisphere(float2 u)
{
    float phi = 2.0f * M_PI_F * u.x;
    
    float cos_phi;
    float sin_phi = sincos(phi, cos_phi);
    
    float cos_theta = sqrt(u.y);
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    
    return float3(sin_theta * cos_phi, cos_theta, sin_theta * sin_phi);
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

struct LightSample
{
    float3 direction;
    float3 color;
    float distance;
};

inline LightSample sampleAreaLight(AreaLight light,
                                   float2 u,
                                   float3 position,
                                   float3 vertexNormal)
{
    LightSample result;
    
    // Map to -1..1
    u = u * 2.0f - 1.0f;
    
    // Transform into light's coordinate system
    float3 samplePosition = light.position +
                            light.right * u.x +
                            light.up * u.y;
    
    // Compute vector from sample point on light source to intersection point
    result.direction = samplePosition - position;
    
    result.distance = length(result.direction);
    
    float inverseLightDistance = 1.0f / max(result.distance, 1e-3f);
    
    // Normalize the light direction
    result.direction *= inverseLightDistance;
    
    // Start with the light's color
    result.color = light.color;
    
    // Light falls off with the inverse square of the distance to the intersection point
    result.color *= (inverseLightDistance * inverseLightDistance);
    
    // Light also falls off with the cosine of angle between the intersection point and
    // the light source
    result.color *= saturate(dot(-result.direction, light.forward));
    
    // Scale the light color by the cosine of the angle between the light direction and
    // surface normal
    result.color *= saturate(dot(vertexNormal, result.direction));
    
    return result;
}

#endif // SHADERS_COMMON_HEADER_GUARD
