#ifndef _RENDERERS_METAL_
#define _RENDERERS_METAL_

#include "../Renderer.h"

class MetalRenderer : public Renderer
{
protected:
    void* _swapchain;
    void* _renderer;
    
public:
    void Init(Platform* platform);
    void Destroy();
    
    void LoadScene(Scene* scene);
    
    void RenderFrame();
};

#endif
