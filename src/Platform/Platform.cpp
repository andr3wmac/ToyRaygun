//
//  Platform.cpp
//  ToyRaygun
//
//  Created by Andrew on 2021-12-22.
//

#include "Platform.h"

#ifdef PLATFORM_WINDOWS
#include "Renderer/D3D12/D3D12Shader.h"
#include "Renderer/D3D12/D3D12Renderer.h"
#else
#include "Renderer/Metal/MetalShader.h"
#include "Renderer/Metal/MetalRenderer.h"
#endif

Shader* Platform::CreateShader()
{
#ifdef PLATFORM_WINDOWS
    D3D12Shader* newShader = new D3D12Shader();
    return newShader;
#else
    MetalShader* newShader = new MetalShader();
    return newShader;
#endif
}

Renderer* Platform::CreateRenderer()
{
#ifdef PLATFORM_WINDOWS
    D3D12Renderer* newRenderer = new D3D12Renderer();
    return newRenderer;
#else
    MetalRenderer* newRenderer = new MetalRenderer();
    return newRenderer;
#endif
}

void Platform::Init()
{
    m_quit = false;
    
#ifdef __APPLE__
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif
    
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("Toy Raygun", 32, 32, 1024, 768, SDL_WINDOW_ALLOW_HIGHDPI);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void Platform::Destroy()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Platform::PollEvents()
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
