#ifndef _RENDERERS_BASE_
#define _RENDERERS_BASE_

#include "Platform.h"
#include "../Scene/Scene.h"

class Renderer
{
public:
    virtual void Init(Platform* platform);
    virtual void Destroy();
    virtual void LoadScene(Scene* scene);
    virtual void RenderFrame();
};

#endif
