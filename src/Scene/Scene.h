#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <simd/simd.h>

using namespace simd;

class Scene
{
protected:
    void createCubeFace(std::vector<float3> & vertices,
                        std::vector<float3> & normals,
                        std::vector<float3> & colors,
                        float3 *cubeVertices,
                        float3 color,
                        unsigned int i0,
                        unsigned int i1,
                        unsigned int i2,
                        unsigned int i3,
                        bool inwardNormals,
                        unsigned int triangleMask);
    
public:
    std::vector<vector_float3> vertices;
    std::vector<vector_float3> normals;
    std::vector<vector_float3> colors;
    std::vector<uint32_t> masks;
    
    void addCube(vector_float3 color,
                 matrix_float4x4 transform);
    
    void addPlane(vector_float3 color,
                  matrix_float4x4 transform);
    
    void addAreaLight(vector_float3 color,
                      matrix_float4x4 transform);
};

#endif /* Scene_h */
