/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "platform/Platform.h"
#include "engine/Renderer.h"
#include "engine/Scene.h"
using namespace toyraygun;

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
    scene->addAreaLight(bx::Vec3(1.0f, 1.0f, 1.0f), transform);
    
    delete[] transform;

    return scene;
}

int main (int argc, char *args[])
{
    Platform* platform = new Platform();
    platform->init(1024, 768);
    
    Shader* rtShader = Platform::createShader();
    if (rtShader->load("Raytracing.shader", false))
    {
        rtShader->addFunction("raygen", ShaderFunctionType::RayGen);
        rtShader->addFunction("primaryHit", ShaderFunctionType::ClosestHit);
        rtShader->addFunction("primaryMiss", ShaderFunctionType::Miss);
        rtShader->addFunction("shadowHit", ShaderFunctionType::ShadowHit);
        rtShader->addFunction("shadowMiss", ShaderFunctionType::ShadowMiss);

        if (!rtShader->compile())
        {
            return -1;
        }
    }
    else {
        // Print error.
    }

    Renderer* renderer = Platform::createRenderer();
    if (!renderer->init(platform))
    {
        return -1;
    }
    renderer->setRaytracingShader(rtShader);
    
    Scene* scene = createCornellBoxScene();
    renderer->loadScene(scene);
    
    while (!platform->hasQuit())
    {
        platform->pollEvents();
        renderer->renderFrame();
    }
    
    return 0;
}
