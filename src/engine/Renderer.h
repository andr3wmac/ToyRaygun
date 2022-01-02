/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef RENDERER_HEADER_GUARD
#define RENDERER_HEADER_GUARD

#include "platform/Platform.h"
#include "engine/Scene.h"
#include "engine/Shader.h"

#include <bx/math.h>

#define MATERIAL_DEFAULT  1
#define MATERIAL_EMISSIVE 2

namespace toyraygun
{
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
        virtual bool init(Platform* platform);
        virtual void destroy();
        virtual void loadScene(Scene* scene);
        virtual Shader* getRaytracingShader();
        virtual void setRaytracingShader(Shader* shader);
        virtual void renderFrame();
    };
}

#endif // RENDERER_HEADER_GUARD
