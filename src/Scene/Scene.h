#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <bx/math.h>

class Scene
{
protected:
    void addGeometry(bx::Vec3* _vertices,
                     uint32_t* _indices,
                     int _triangleCount,
                     float* transformMtx,
                     bx::Vec3 _color,
                     unsigned int _mask);
    
public:
    std::vector<bx::Vec3> vertices;
    std::vector<uint32_t> indices;
    std::vector<bx::Vec3> normals;
    std::vector<bx::Vec3> colors;
    std::vector<uint32_t> masks;
    
    void addCube(bx::Vec3 color, float* transform);
    void addPlane(bx::Vec3 color, float* transform);
    void addAreaLight(bx::Vec3 color, float* transform);
};

#endif /* Scene_h */
