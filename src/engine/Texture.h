/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef TEXTURE_HEADER_GUARD
#define TEXTURE_HEADER_GUARD

#include <vector>
#include <bx/math.h>

namespace toyraygun
{
    class Texture
    {
    protected:
        void* m_data;
        int m_width;
        int m_height;
        int m_channels;
        
    public:
        static Texture generateRandomTexture(int width, int height);
        
        virtual void init(int width, int height, int channels);
        virtual void destroy();
        virtual uint32_t* getBufferPointer();
        
        virtual int getWidth();
        virtual int getHeight();
        virtual int getChannels();
    };
}

#endif // TEXTURE_HEADER_GUARD