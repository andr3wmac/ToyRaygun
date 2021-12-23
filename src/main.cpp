/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Platform/Platform.h"
#include "Renderer/Metal/MetalRenderer.h"
#include "Renderer/ShaderTypes.h"
#include "Scene/Scene.h"
#include "Scene/Transforms.h"

using namespace simd;

# define PI 3.14159265358979323846

Scene* createCornellBoxScene()
{
    Scene* scene = new Scene();
    
    float4x4 transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_rotation(PI, vector3(0.0f, 1.0f, 0.0f)) * matrix4x4_scale(0.5f, 1.98f, 0.5f);
    
    // Short box
    transform = matrix4x4_translation(0.3275f, 0.3f, 0.3725f) * matrix4x4_rotation(-0.3f, vector3(0.0f, 1.0f, 0.0f)) * matrix4x4_scale(0.6f, 0.6f, 0.6f);
    scene->addCube(vector3(0.725f, 0.71f, 0.68f), transform);
    
    // Tall box
    transform = matrix4x4_translation(-0.335f, 0.6f, -0.29f) * matrix4x4_rotation(0.3f, vector3(0.0f, 1.0f, 0.0f)) * matrix4x4_scale(0.6f, 1.2f, 0.6f);
    scene->addCube(vector3(0.725f, 0.71f, 0.68f), transform);
    
    // Ceiling
    transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_rotation(PI, vector3(0.0f, 0.0f, 1.0f)) * matrix4x4_scale(2.0f, 2.0f, 2.0f);
    scene->addPlane(vector3(0.725f, 0.71f, 0.68f), transform);
    
    // Floor
    transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_scale(2.0f, 2.0f, 2.0f);
    scene->addPlane(vector3(0.725f, 0.71f, 0.68f), transform);
    
    // Left Wall
    transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_rotation(-PI / 2, vector3(0.0f, 0.0f, 1.0f)) * matrix4x4_scale(2.0f, 2.0f, 2.0f);
    scene->addPlane(vector3(0.63f, 0.065f, 0.05f), transform);
    
    // Right Wall
    transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_rotation(PI / 2, vector3(0.0f, 0.0f, 1.0f)) * matrix4x4_scale(2.0f, 2.0f, 2.0f);
    scene->addPlane(vector3(0.14f, 0.05f, 0.491f), transform);
    
    // Back Wall
    transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_rotation(PI / 2, vector3(1.0f, 0.0f, 0.0f)) * matrix4x4_scale(2.0f, 2.0f, 2.0f);
    scene->addPlane(vector3(0.725f, 0.71f, 0.68f), transform);
    
    // Light source
    transform = matrix4x4_translation(0.0f, 1.0f, 0.0f) * matrix4x4_rotation(PI, vector3(0.0f, 0.0f, 1.0f)) * matrix4x4_scale(0.5f, 1.98f, 0.5f);
    scene->addAreaLight(vector3(1.0f, 0.0f, 1.0f), transform);
    
    return scene;
}

int main (int argc, char *args[])
{
    Platform* platform = new Platform();
    platform->Init();
    
    Renderer* renderer = new MetalRenderer();
    renderer->Init(platform);
    
    Scene* scene = createCornellBoxScene();
    renderer->LoadScene(scene);
    
    while (!platform->quit)
    {
        platform->PollEvents();
        renderer->RenderFrame();
    }
    
    return 0;
}
