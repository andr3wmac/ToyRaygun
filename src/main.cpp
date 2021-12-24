/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Platform/Platform.h"
#include "Renderer/ShaderTypes.h"
#include "Scene/Scene.h"
#include "bx/math.h"

#include "Scene/Transforms.h"
#include <iostream>

#ifdef PLATFORM_WINDOWS
#include "Renderer/D3D12/D3D12Renderer.h"
#else
#include "Renderer/Metal/MetalRenderer.h"
#endif

using namespace simd;

Scene* createCornellBoxScene()
{
    Scene* scene = new Scene();
    
    float transform[16];

    float4x4 origTransform = matrix4x4_translation(0.3275f, 0.3f, 0.3725f) * matrix4x4_rotation(-0.3f, vector3(0.0f, 1.0f, 0.0f)) * matrix4x4_scale(0.6f, 0.6f, 0.6f);
    
    // Short box
    bx::mtxSRT(transform, 0.6f, 0.6f, 0.6f, 0.0f, -0.3f, 0.0f, 0.3275f, 0.3f, 0.3725f);
    scene->addCube(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    std::cout << "Original: " << std::endl;
    for (int i = 0; i < 4; ++i)
    {
        std::cout << origTransform.columns[i].x << " ";
        std::cout << origTransform.columns[i].y << " ";
        std::cout << origTransform.columns[i].z << " ";
        std::cout << origTransform.columns[i].w << std::endl;
    }
    
    std::cout << "New: " << std::endl;
    for (int i = 0; i < 4; i++)
    {
        std::cout << transform[(i * 4) + 0] << " ";
        std::cout << transform[(i * 4) + 1] << " ";
        std::cout << transform[(i * 4) + 2] << " ";
        std::cout << transform[(i * 4) + 3] << std::endl;
    }
    
    // Tall box
    bx::mtxSRT(transform, 0.6f, 1.2f, 0.6f, 0.0f, 0.3f, 0.0f, -0.335f, 0.6f, -0.29f);
    scene->addCube(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Ceiling
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, bx::kPi, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Floor
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Left Wall
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, -bx::kPi / 2.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.63f, 0.065f, 0.05f), transform);
    
    // Right Wall
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, bx::kPi / 2.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.14f, 0.05f, 0.491f), transform);
    
    // Back Wall
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, bx::kPi / 2.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Light source
    bx::mtxSRT(transform, 0.5f, 1.98f, 0.5f, 0.0f, 0.0f, bx::kPi, 0.0f, 1.0f, 0.0f);
    scene->addAreaLight(bx::Vec3(1.0f, 0.0f, 1.0f), transform);
    
    return scene;
}

int main (int argc, char *args[])
{
    Platform* platform = new Platform();
    platform->Init();
    
    Renderer* renderer = nullptr;

#ifdef PLATFORM_WINDOWS
    renderer = new D3D12Renderer();
#else
    renderer = new MetalRenderer();
#endif

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
