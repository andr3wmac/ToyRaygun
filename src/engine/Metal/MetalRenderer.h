/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef _RENDERERS_METAL_
#define _RENDERERS_METAL_

#include "engine/Renderer.h"

namespace toyraygun
{
    class MetalRenderer : public Renderer
    {
    protected:
        void* _swapchain;
        void* _renderer;
        
    public:
        bool init();
        void destroy();
        
        void loadScene(Scene* scene);
        
        void renderFrame();
    };
}

#endif
