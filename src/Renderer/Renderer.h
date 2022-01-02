#ifndef _RENDERERS_BASE_
#define _RENDERERS_BASE_

#include "Platform/Platform.h"
#include "Scene/Scene.h"
#include "Shader.h"
#include <bx/math.h>

#define TRIANGLE_MASK_GEOMETRY 1
#define TRIANGLE_MASK_LIGHT    2

struct Camera {
    bx::Vec3 position;
    bx::Vec3 right;
    bx::Vec3 up;
    bx::Vec3 forward;
};

struct AreaLight {
    bx::Vec3 position;
    bx::Vec3 forward;
    bx::Vec3 right;
    bx::Vec3 up;
    bx::Vec3 color;
};

struct Uniforms
{
    unsigned int width;
    unsigned int height;
    unsigned int frameIndex;
    Camera camera;
    AreaLight light;
};

class Renderer
{
protected:
    Shader* m_rtShader;

public:
    virtual void init(Platform* platform);
    virtual void destroy();
    virtual void loadScene(Scene* scene);
    virtual void setRaytracingShader(Shader* shader);
    virtual void renderFrame();
};

#endif
