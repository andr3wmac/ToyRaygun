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
