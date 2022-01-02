/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Platform/Platform.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"

#include <bx/math.h>

Scene* createCornellBoxScene()
{
    Scene* scene = new Scene();
    
    float* transform = new float[16];
    
    // Short box
    bx::mtxSRT(transform, 0.6f, 0.6f, 0.6f, 0.0f, 0.3f, 0.0f, 0.3275f, 0.3f, 0.3725f);
    scene->addCube(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Tall box
    bx::mtxSRT(transform, 0.6f, 1.2f, 0.6f, 0.0f, -0.3f, 0.0f, -0.335f, 0.6f, -0.29f);
    scene->addCube(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Ceiling
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, bx::kPi, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Floor
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.725f, 0.71f, 0.68f), transform);
    
    // Left Wall
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, bx::kPi / 2.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.63f, 0.065f, 0.05f), transform);
    
    // Right Wall
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, -bx::kPi / 2.0f, 0.0f, 1.0f, 0.0f);
    scene->addPlane(bx::Vec3(0.14f, 0.491f, 0.05f), transform);
    
    // Back Wall
    bx::mtxSRT(transform, 2.0f, 2.0f, 2.0f, -bx::kPi / 2.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
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
    
    Shader* rtShader = Platform::CreateShader();
    if (rtShader->Load("shaders/d3d12/Raytracing.shader", false))
    {
        rtShader->AddFunction("MyRaygenShader", ShaderFunctionType::RayGen);
        rtShader->AddFunction("MyClosestHitShader", ShaderFunctionType::ClosestHit);
        rtShader->AddFunction("MyMissShader", ShaderFunctionType::Miss);

        if (!rtShader->Compile())
        {
            return -1;
        }
    }
    else {
        // Print error.
    }

    Renderer* renderer = Platform::CreateRenderer();
    renderer->Init(platform);
    renderer->SetRaytracingShader(rtShader);
    
    Scene* scene = createCornellBoxScene();
    renderer->LoadScene(scene);
    
    while (!platform->m_quit)
    {
        platform->PollEvents();
        renderer->RenderFrame();
    }
    
    return 0;
}
