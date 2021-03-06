/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef RENDERER_HEADER_GUARD
#define RENDERER_HEADER_GUARD

#include "engine/Engine.h"
#include "engine/Scene.h"
#include "engine/Shader.h"
#include "engine/Uniforms.h"

#include <bx/math.h>

#define MATERIAL_DEFAULT  1
#define MATERIAL_EMISSIVE 2

namespace toyraygun
{
    class Renderer
    {
    protected:
        int m_frameIndex;
        std::vector<Shader*> m_shaders;

        // Viewport dimensions.
        int m_width;
        int m_height;
        float m_aspectRatio;

        // Camera
        bx::Vec3 m_eye;
        bx::Vec3 m_up;
        bx::Vec3 m_at;
        float m_viewMtx[16];
        float m_projMtx[16];
        float m_viewProjMtx[16];

    public:
        Renderer();

        virtual bool init();
        virtual void destroy();
        virtual void loadScene(Scene* scene);
        virtual void renderFrame();

        // Camera
        void getViewProjMtx(float* mtxOut);
        bx::Vec3 getCameraPosition();
        void setCameraPosition(bx::Vec3 position);
        void setCameraLookAt(bx::Vec3 position);
        void updateCamera(); 

        virtual void addShader(Shader* shader);
        virtual Shader* getShader(std::string path);
    };
}

#endif // RENDERER_HEADER_GUARD
