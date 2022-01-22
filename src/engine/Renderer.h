/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef RENDERER_HEADER_GUARD
#define RENDERER_HEADER_GUARD

#include "engine/Platform.h"
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
        Shader* m_rtShader;
        Shader* m_accumulateShader;
        Shader* m_postProcessingShader;

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

        virtual bool init(Platform* platform);
        virtual void destroy();
        virtual void loadScene(Scene* scene);
        virtual void renderFrame();

        // Camera
        void getViewProjMtx(float* mtxOut);
        bx::Vec3 getCameraPosition();
        void setCameraPosition(bx::Vec3 position);
        void setCameraLookAt(bx::Vec3 position);
        void updateCamera();

        virtual Shader* getRaytracingShader();
        virtual void setRaytracingShader(Shader* shader);
        virtual Shader* getAccumulateShader();
        virtual void setAccumulateShader(Shader* shader);
        virtual Shader* getPostProcessingShader();
        virtual void setPostProcessingShader(Shader* shader);
    };
}

#endif // RENDERER_HEADER_GUARD
