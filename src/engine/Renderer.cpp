/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Renderer.h"
using namespace toyraygun;

bool Renderer::init(Platform* platform)
{
	return false;
}

void Renderer::destroy()
{

}

void Renderer::loadScene(Scene* scene)
{

}

void Renderer::setRaytracingShader(Shader* shader)
{
	m_rtShader = shader;
}

void Renderer::renderFrame()
{

}
