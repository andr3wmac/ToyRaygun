/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "engine/Platform.h"
#include "engine/Renderer.h"
#include "engine/Scene.h"

using namespace toyraygun;

#include <iostream>
#include <bx/math.h>
#include "cornellBox.h"

int main (int argc, char *args[])
{
    // Initialize platform and window.
    Platform* platform = new Platform();
    platform->init(1024, 768);

    // Setup renderer with automatic selection based on platform.
    Renderer* renderer = Platform::createRenderer();
    if (!renderer->init(platform))
    {
        std::cout << "Failed to initialize platform." << std::endl;
        return -1;
    }
    
    // Setup camera.
    renderer->setCameraPosition(bx::Vec3(0.0f, 1.0f, 3.38f));
    renderer->setCameraLookAt(bx::Vec3(0.0f, 1.0f, -1.0f));
    
    // Setup raytracing shader.
    Shader* rtShader = Platform::createShader();
    if (rtShader->load("Raytracing"))
    {
        rtShader->addFunction("raygen",      ShaderFunctionType::RayGen);
        rtShader->addFunction("primaryHit",  ShaderFunctionType::ClosestHit);
        rtShader->addFunction("primaryMiss", ShaderFunctionType::Miss);
        rtShader->addFunction("shadowHit",   ShaderFunctionType::ShadowHit);
        rtShader->addFunction("shadowMiss",  ShaderFunctionType::ShadowMiss);

        if (!rtShader->compile())
        {
            std::cout << "Failed to compile raytracing shader." << std::endl;
            return -1;
        }
    }
    else
    {
        std::cout << "Failed to load raytracing shader." << std::endl;
        return -1;
    }
    
    renderer->setRaytracingShader(rtShader);

    // Load cornell box scene.
    Scene* scene = createCornellBoxScene();
    renderer->loadScene(scene);
    
    // Render loop.
    while (!platform->hasQuit())
    {
        platform->pollEvents();
        renderer->renderFrame();
    }
    
    return 0;
}
