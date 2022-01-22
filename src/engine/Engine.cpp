/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Engine.h"
using namespace toyraygun;

#ifdef PLATFORM_WINDOWS
#include "engine/D3D12/D3D12Shader.h"
#include "engine/D3D12/D3D12Renderer.h"
#else
#include "engine/Metal/MetalShader.h"
#include "engine/Metal/MetalRenderer.h"
#endif

Engine* Engine::m_instance = nullptr;

Engine* Engine::instance()
{
    if (m_instance == nullptr)
    {
        m_instance = new Engine();
    }
    
    return m_instance;
}

Shader* Engine::createShader()
{
#ifdef PLATFORM_WINDOWS
    D3D12Shader* newShader = new D3D12Shader();
    return newShader;
#else
    MetalShader* newShader = new MetalShader();
    return newShader;
#endif
}

Renderer* Engine::createRenderer()
{
#ifdef PLATFORM_WINDOWS
    D3D12Renderer* newRenderer = new D3D12Renderer();
    return newRenderer;
#else
    MetalRenderer* newRenderer = new MetalRenderer();
    return newRenderer;
#endif
}

std::string Engine::getRuntimeShaderPath()
{
#ifdef PLATFORM_WINDOWS
    return "shaders/d3d12/";
#else
    return "shaders/metal/";
#endif
}

std::string Engine::getRuntimeShaderExt()
{
#ifdef PLATFORM_WINDOWS
    return "hlsl";
#else
    return "metal";
#endif
}

void Engine::init(int width, int height)
{
    m_quit = false;
    m_width = width;
    m_height = height;
    
#ifdef __APPLE__
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif
    
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("Toy Raygun", 32, 32, m_width, m_height, SDL_WINDOW_ALLOW_HIGHDPI);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void Engine::destroy()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

int Engine::getWidth()
{
    return m_width;
}

int Engine::getHeight()
{
    return m_height;
}

bool Engine::hasQuit()
{
    return m_quit;
}

void Engine::pollEvents()
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
