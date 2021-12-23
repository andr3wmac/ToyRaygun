//
//  Platform.hpp
//  ToyRaygun
//
//  Created by Andrew on 2021-12-22.
//

#ifndef Platform_H
#define Platform_H

#include <stdio.h>
#include <SDL.h>

#if defined(_MSC_VER)
#    if defined(_WIN32)
#        define PLATFORM_WINDOWS 1
#        if defined(_WIN64)
#           define PLATFORM_WINDOWS_64 1
#        endif
#    endif
#endif

class Platform
{
protected:
    SDL_Window* window;
    SDL_Renderer* renderer;
    
public:
    bool quit;

    virtual void Init();
    virtual void Destroy();
    
    virtual void PollEvents();
    
    SDL_Renderer* getRenderer() { return renderer; }
};

#endif /* Platform_H */
