/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Renderer.h"
using namespace toyraygun;

Renderer::Renderer() :
    m_frameIndex(0),
    m_eye(0.0f, 0.0, 0.0f),
    m_up(0.0f, 1.0, 0.0f),
    m_at(0.0f, 0.0, 0.0f)
{

}

bool Renderer::init()
{
    Engine* engine = Engine::instance();
    
    m_width = engine->getWidth();
    m_height = engine->getHeight();
    m_aspectRatio = float(m_width) / float(m_height);

	return false;
}

void Renderer::destroy()
{

}

void Renderer::loadScene(Scene* scene)
{

}

void Renderer::addShader(Shader* shader)
{
    m_shaders.push_back(shader);
}

Shader* Renderer::getShader(std::string path)
{
    for (int i = 0; i < m_shaders.size(); ++i)
    {
        if (m_shaders[i]->m_path == path)
        {
            return m_shaders[i];
        }
    }

    return nullptr;
}

void Renderer::renderFrame()
{
    m_frameIndex++;
}

void Renderer::getViewProjMtx(float* mtxOut)
{
    memcpy(mtxOut, m_viewProjMtx, sizeof(m_viewProjMtx));
}

bx::Vec3 Renderer::getCameraPosition()
{
    return m_eye;
}

void Renderer::setCameraPosition(bx::Vec3 position)
{
    m_eye = position;
    updateCamera();
}

void Renderer::setCameraLookAt(bx::Vec3 position)
{
    m_at = position;
    updateCamera();
}

void Renderer::updateCamera()
{
    bx::mtxLookAt(m_viewMtx, m_eye, m_at, m_up, bx::Handness::Right);
    bx::mtxProj(m_projMtx, 45.0f, m_aspectRatio, 1.0f, 125.0f, true, bx::Handness::Right);
    bx::mtxMul(m_viewProjMtx, m_viewMtx, m_projMtx);
}
