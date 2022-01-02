/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef SCENE_HEADER_GUARD
#define SCENE_HEADER_GUARD

#include <vector>
#include <bx/math.h>

namespace toyraygun
{
    class Scene
    {
    protected:
        void addGeometry(bx::Vec3* vertices,
            uint32_t* indices,
            int triangleCount,
            float* transformMtx,
            bx::Vec3 color,
            unsigned int materialID);

    public:
        std::vector<bx::Vec3> m_vertexBuffer;
        std::vector<uint32_t> m_indexBuffer;
        std::vector<bx::Vec3> m_normalBuffer;
        std::vector<bx::Vec3> m_colorBuffer;
        std::vector<uint32_t> m_materialIDBuffer;

        // Geometry
        void addCube(bx::Vec3 color, float* transformMtx);
        void addPlane(bx::Vec3 color, float* transformMtx);
        void addAreaLight(bx::Vec3 color, float* transformMtx);
    };
}

#endif // SCENE_HEADER_GUARD
