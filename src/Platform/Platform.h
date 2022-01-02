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
#undef main

#if defined(_MSC_VER)
#    if defined(_WIN32)
#        define PLATFORM_WINDOWS 1
#        if defined(_WIN64)
#           define PLATFORM_WINDOWS_64 1
#        endif
#    endif
#endif

namespace toyraygun
{
    class Shader;
    class Renderer;

    class Platform
    {
    protected:
        bool m_quit;
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;

    public:
        static Shader* createShader();
        static Renderer* createRenderer();

        virtual void init();
        virtual void destroy();

        virtual bool hasQuit();
        virtual void pollEvents();

        SDL_Renderer* getRenderer() { return m_renderer; }
    };
}

#endif /* Platform_H */
