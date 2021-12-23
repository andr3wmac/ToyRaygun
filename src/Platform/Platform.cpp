//
//  Platform.cpp
//  ToyRaygun
//
//  Created by Andrew on 2021-12-22.
//

#include "Platform.h"

void Platform::Init()
{
    quit = false;
    
#ifdef __APPLE__
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif
    
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Toy Raygun", -1, -1, 1024, 768, SDL_WINDOW_ALLOW_HIGHDPI);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void Platform::Destroy()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
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
                quit = true;
                break;
        }
    }
}
