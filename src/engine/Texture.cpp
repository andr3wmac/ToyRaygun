/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Texture.h"
#include "engine/Renderer.h"
using namespace toyraygun;

#include <iostream>
#include <bx/math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture Texture::generateRandomTexture(int width, int height, int channels)
{
    Texture randomTexture;
    randomTexture.init(width, height, channels);
    
    uint8_t* randomValues = randomTexture.getBufferPointer();
    
    for (int i = 0; i < (width * height * channels); i++)
    {
        randomValues[i] = rand() % (1024 * 1024);
    }
    
    return randomTexture;
}

void Texture::init(int width, int height, int channels)
{
    m_data = (uint8_t *)malloc(sizeof(uint8_t) * width * height * channels);
    m_width = width;
    m_height = height;
    m_channels = channels;
}

bool Texture::loadFile(std::string path)
{
    m_data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (m_data == NULL)
    {
        return false;
    }

    return true;
}

void Texture::destroy()
{
    if (m_data != nullptr)
    {
        free(m_data);
    }
}

uint8_t* Texture::getBufferPointer()
{
    return (uint8_t*)(m_data);
}

size_t Texture::getBufferSize()
{
    return sizeof(uint8_t) * m_width * m_height * m_channels;
}

size_t Texture::getBufferStride()
{
    return sizeof(uint8_t) * m_width * m_channels;
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
