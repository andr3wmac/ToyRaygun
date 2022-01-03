/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Texture.h"
#include "engine/Renderer.h"
using namespace toyraygun;

#include <iostream>
#include <bx/math.h>

Texture Texture::generateRandomTexture(int width, int height)
{
    Texture randomTexture;
    randomTexture.init(width, height, 1);
    
    uint32_t* randomValues = randomTexture.getBufferPointer();
    
    for (int i = 0; i < width * height; i++)
    {
        randomValues[i] = rand() % (1024 * 1024);
    }
    
    return randomTexture;
}

void Texture::init(int width, int height, int channels)
{
    m_data = (uint32_t *)malloc(sizeof(uint32_t) * width * height * channels);
    m_width = width;
    m_height = height;
    m_channels = channels;
}

void Texture::destroy()
{
    if (m_data != nullptr)
    {
        free(m_data);
    }
}

uint32_t* Texture::getBufferPointer()
{
    return (uint32_t*)(m_data);
}

int Texture::getWidth()
{
    return m_width;
}

int Texture::getHeight()
{
    return m_height;
}

int Texture::getChannels()
{
    return m_channels;
}
