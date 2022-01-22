/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "engine/Platform.h"
#include "engine/Renderer.h"
#include "engine/Scene.h"
using namespace toyraygun;

#include <bx/math.h>
#include <iostream>

#include "cornellBox.h"

int main (int argc, char *args[])
{
    Platform* platform = new Platform();
    platform->init(1024, 768);
    
    Shader* rtShader = Platform::createShader();
    if (rtShader->load("Raytracing"))
    {
        rtShader->addFunction("raygen", ShaderFunctionType::RayGen);
        rtShader->addFunction("primaryHit", ShaderFunctionType::ClosestHit);
        rtShader->addFunction("primaryMiss", ShaderFunctionType::Miss);
        rtShader->addFunction("shadowHit", ShaderFunctionType::ShadowHit);
        rtShader->addFunction("shadowMiss", ShaderFunctionType::ShadowMiss);

        if (!rtShader->compile())
        {
            std::cout << "Failed to compile Raytracing shader." << std::endl;
            return -1;
        }
    }
    else {
        // Print error.
    }

    Shader* accumulateShader = Platform::createShader();
    if (accumulateShader->load("Accumulate"))
    {
        accumulateShader->addFunction("accumulate", ShaderFunctionType::Accumulate);

        if (!accumulateShader->compile("main"))
        {
            std::cout << "Failed to compile Accumulate shader." << std::endl;
            //return -1;
        }
    }
    else {
        // Print error.
    }

    Shader* postProcessingShader = Platform::createShader();
    if (postProcessingShader->load("PostProcessing"))
    {
        postProcessingShader->addFunction("postprocessing", ShaderFunctionType::PostProcessing);

        if (!postProcessingShader->compile("main"))
        {
            std::cout << "Failed to compile PostProcessing shader." << std::endl;
            //return -1;
        }
    }
    else {
        // Print error.
    }

    Renderer* renderer = Platform::createRenderer();
    if (!renderer->init(platform))
    {
        std::cout << "Renderer failed to initialize." << std::endl;
        return -1;
    }
    renderer->setRaytracingShader(rtShader);
    renderer->setAccumulateShader(accumulateShader);
    renderer->setPostProcessingShader(postProcessingShader);
    renderer->setCameraPosition(bx::Vec3(0.0f, 1.0f, 3.38f));
    renderer->setCameraLookAt(bx::Vec3(0.0f, 1.0f, -1.0f));

    Scene* scene = createCornellBoxScene();
    renderer->loadScene(scene);
    
    while (!platform->hasQuit())
    {
        platform->pollEvents();
        renderer->renderFrame();
    }
    
    return 0;
}
