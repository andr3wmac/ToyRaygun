/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "engine/Engine.h"
#include "engine/Renderer.h"
#include "engine/Shader.h"
using namespace toyraygun;

#include <bx/math.h>
#include <iostream>

#include "cornellBox.h"

int main (int argc, char *args[])
{
    // Uncomment to load PIX debugging DLL.
    // Engine::initPIXDebugger();

    Engine* engine = Engine::instance();
    engine->init(1024, 768);
    
    Shader* rtShader = Engine::createShader();
    if (rtShader->load("Raytracing"))
    {
        rtShader->addFunction("raygen", ShaderFunctionType::RayGen);
        rtShader->addFunction("primaryHit", ShaderFunctionType::ClosestHit);
        rtShader->addFunction("primaryMiss", ShaderFunctionType::Miss);
        rtShader->addFunction("shadowHit", ShaderFunctionType::ShadowHit);
        rtShader->addFunction("shadowMiss", ShaderFunctionType::ShadowMiss);

        if (!rtShader->compile(ShaderType::Raytrace))
        {
            std::cout << "Failed to compile Raytracing shader." << std::endl;
            return -1;
        }
    }
    else {
        // Print error.
    }

    Shader* accumulateShader = Engine::createShader();
    if (accumulateShader->load("Accumulate"))
    {
        accumulateShader->addFunction("accumulate", ShaderFunctionType::Compute);

        if (!accumulateShader->compile(ShaderType::Compute))
        {
            std::cout << "Failed to compile Accumulate shader." << std::endl;
            return -1;
        }
    }
    else {
        // Print error.
    }

    Shader* postProcessingShader = Engine::createShader();
    if (postProcessingShader->load("PostProcessing"))
    {
        postProcessingShader->addFunction("vert", ShaderFunctionType::Vertex);
        postProcessingShader->addFunction("frag", ShaderFunctionType::Fragment);
        
        // temporary until dx12 is converted
        postProcessingShader->addFunction("postProcess", ShaderFunctionType::Compute);

        if (!postProcessingShader->compile(ShaderType::Graphics))
        {
            std::cout << "Failed to compile PostProcessing shader." << std::endl;
            return -1;
        }
    }
    else {
        // Print error.
    }

    Renderer* renderer = Engine::createRenderer();
    if (!renderer->init())
    {
        std::cout << "Renderer failed to initialize." << std::endl;
        return -1;
    }

    renderer->addShader(rtShader);
    renderer->addShader(accumulateShader);
    renderer->addShader(postProcessingShader);

    renderer->setCameraPosition(bx::Vec3(0.0f, 1.0f, 3.38f));
    renderer->setCameraLookAt(bx::Vec3(0.0f, 1.0f, -1.0f));

    Scene* scene = createCornellBoxScene();
    renderer->loadScene(scene);
    
    while (!engine->hasQuit())
    {
        engine->pollEvents();
        renderer->renderFrame();
    }
    
    return 0;
}
