/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef ENGINE_HEADER_GUARD
#define ENGINE_HEADER_GUARD

#include <string>
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

    class Engine
    {
    protected:
        static Engine* m_instance;
        
        int m_width;
        int m_height;
        bool m_quit;
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
        
    public:
        static Engine* instance();
        static void initPIXDebugger();
        
        static Shader* createShader();
        static Renderer* createRenderer();
        static std::string getRuntimeShaderPath();
        static std::string getRuntimeShaderExt();

        virtual void init(int width, int height);
        virtual void destroy();

        virtual int getWidth();
        virtual int getHeight();
        virtual bool hasQuit();
        virtual void pollEvents();

        SDL_Renderer* getRenderer() { return m_renderer; }
    };
}

#endif // ENGINE_HEADER_GUARD
