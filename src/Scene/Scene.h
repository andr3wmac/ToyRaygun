#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <bx/math.h>

class Scene
{
protected:
    void addGeometry(bx::Vec3* vertices,
                     uint32_t* indices,
                     int triangleCount,
                     float* transformMtx,
                     bx::Vec3 color,
                     unsigned int mask);
    
public:
    std::vector<bx::Vec3> vertexBuffer;
    std::vector<uint32_t> indexBuffer;
    std::vector<bx::Vec3> normalBuffer;
    std::vector<bx::Vec3> colorBuffer;
    std::vector<uint32_t> maskBuffer;
    
    void addCube(bx::Vec3 color, float* transform);
    void addPlane(bx::Vec3 color, float* transform);
    void addAreaLight(bx::Vec3 color, float* transform);
};

#endif /* Scene_h */
