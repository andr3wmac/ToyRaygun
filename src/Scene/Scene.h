#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <simd/simd.h>
#include <bx/math.h>

using namespace simd;

class Scene
{
protected:
    void createCubeFace(std::vector<bx::Vec3>& vertices,
                        std::vector<bx::Vec3>& normals,
                        std::vector<bx::Vec3>& colors,
                        bx::Vec3 *cubeVertices,
                        bx::Vec3 color,
                        unsigned int i0,
                        unsigned int i1,
                        unsigned int i2,
                        unsigned int i3,
                        bool inwardNormals,
                        unsigned int triangleMask);
    
public:
    std::vector<bx::Vec3> vertices;
    std::vector<bx::Vec3> normals;
    std::vector<bx::Vec3> colors;
    std::vector<uint32_t> masks;
    
    void addCube(bx::Vec3 color, float* transform);
    void addPlane(bx::Vec3 color, float* transform);
    void addAreaLight(bx::Vec3 color, float* transform);
};

#endif /* Scene_h */
