/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Platform.h"
using namespace toyraygun;

#ifdef PLATFORM_WINDOWS
#include "engine/D3D12/D3D12Shader.h"
#include "engine/D3D12/D3D12Renderer.h"
#else
#include "engine/Metal/MetalShader.h"
#include "engine/Metal/MetalRenderer.h"
#endif

Shader* Platform::createShader()
{
#ifdef PLATFORM_WINDOWS
    D3D12Shader* newShader = new D3D12Shader();
    return newShader;
#else
    MetalShader* newShader = new MetalShader();
    return newShader;
#endif
}

Renderer* Platform::createRenderer()
{
#ifdef PLATFORM_WINDOWS
    D3D12Renderer* newRenderer = new D3D12Renderer();
    return newRenderer;
#else
    MetalRenderer* newRenderer = new MetalRenderer();
    return newRenderer;
#endif
}

std::string Platform::getRuntimeShaderPath()
{
#ifdef PLATFORM_WINDOWS
    return "shaders/d3d12/";
#else
    return "shaders/metal/";
#endif
}

void Platform::init()
{
    m_quit = false;
    
#ifdef __APPLE__
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif
    
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("Toy Raygun", 32, 32, 1024, 768, SDL_WINDOW_ALLOW_HIGHDPI);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void Platform::destroy()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Platform::hasQuit()
{
    return m_quit;
}

void Platform::pollEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
        switch (e.type)
        {
            case SDL_QUIT:
                m_quit = true;
                break;
        }
    }
}
