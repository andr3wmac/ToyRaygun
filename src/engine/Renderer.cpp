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

bool Renderer::init(Platform* platform)
{
    m_width = platform->getWidth();
    m_height = platform->getHeight();
    m_aspectRatio = float(m_width) / float(m_height);

	return false;
}

void Renderer::destroy()
{

}

void Renderer::loadScene(Scene* scene)
{

}

Shader* Renderer::getRaytracingShader()
{
    return m_rtShader;
}

void Renderer::setRaytracingShader(Shader* shader)
{
	m_rtShader = shader;
}

void Renderer::setAccumulateShader(Shader* shader)
{
    m_accumulateShader = shader;
}

void Renderer::setPostProcessingShader(Shader* shader)
{
    m_postProcessingShader = shader;
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
