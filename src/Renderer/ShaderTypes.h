/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Header containing types and enum constants shared between Metal shaders and Swift/ObjC source
*/

#ifndef ShaderTypes_h
#define ShaderTypes_h

#include "bx/math.h"

#define TRIANGLE_MASK_GEOMETRY 1
#define TRIANGLE_MASK_LIGHT    2

#define RAY_MASK_PRIMARY   3
#define RAY_MASK_SHADOW    1
#define RAY_MASK_SECONDARY 1

struct Camera {
    bx::Vec3 position;
    bx::Vec3 right;
    bx::Vec3 up;
    bx::Vec3 forward;
};

struct AreaLight {
    bx::Vec3 position;
    bx::Vec3 forward;
    bx::Vec3 right;
    bx::Vec3 up;
    bx::Vec3 color;
};

struct Uniforms
{
    unsigned int width;
    unsigned int height;
    unsigned int frameIndex;
    Camera camera;
    AreaLight light;
};

#endif /* ShaderTypes_h */

