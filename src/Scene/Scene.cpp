/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Implementation for scene creation functions
*/


#include "Scene.h"
#include "ShaderTypes.h"
#include <bx/math.h>

using namespace simd;

float3 getTriangleNormal(float3 v0, float3 v1, float3 v2) {
    float3 e1 = normalize(v1 - v0);
    float3 e2 = normalize(v2 - v0);
    
    return cross(e1, e2);
}

void Scene::addCube(bx::Vec3 color, float* transformMtx)
{
    float3 cubeVertices[] = {
        vector3(-0.5f, -0.5f, -0.5f),
        vector3( 0.5f, -0.5f, -0.5f),
        vector3(-0.5f,  0.5f, -0.5f),
        vector3( 0.5f,  0.5f, -0.5f),
        vector3(-0.5f, -0.5f,  0.5f),
        vector3( 0.5f, -0.5f,  0.5f),
        vector3(-0.5f,  0.5f,  0.5f),
        vector3( 0.5f,  0.5f,  0.5f),
    };
    
    for (int i = 0; i < 8; i++) {
        float3 vertex = cubeVertices[i];
        
        float transformedVertex[4] = { vertex.x, vertex.y, vertex.z, 1.0f };
        bx::vec4MulMtx(transformedVertex, transformedVertex, transformMtx);

        cubeVertices[i] = vector3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
    }
    
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 4, 6, 2, false, TRIANGLE_MASK_GEOMETRY);
    createCubeFace(vertices, normals, colors, cubeVertices, color, 1, 3, 7, 5, false, TRIANGLE_MASK_GEOMETRY);
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 1, 5, 4, false, TRIANGLE_MASK_GEOMETRY);
    createCubeFace(vertices, normals, colors, cubeVertices, color, 2, 6, 7, 3, false, TRIANGLE_MASK_GEOMETRY);
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 2, 3, 1, false, TRIANGLE_MASK_GEOMETRY);
    createCubeFace(vertices, normals, colors, cubeVertices, color, 4, 5, 7, 6, false, TRIANGLE_MASK_GEOMETRY);
}

void Scene::addPlane(bx::Vec3 color, float* transformMtx)
{
    float3 cubeVertices[] = {
        vector3(-0.5f, -0.5f, -0.5f),
        vector3( 0.5f, -0.5f, -0.5f),
        vector3(-0.5f,  0.5f, -0.5f),
        vector3( 0.5f,  0.5f, -0.5f),
        vector3(-0.5f, -0.5f,  0.5f),
        vector3( 0.5f, -0.5f,  0.5f),
        vector3(-0.5f,  0.5f,  0.5f),
        vector3( 0.5f,  0.5f,  0.5f),
    };
    
    for (int i = 0; i < 8; i++) {
        float3 vertex = cubeVertices[i];
        
        float transformedVertex[4] = { vertex.x, vertex.y, vertex.z, 1.0f };
        float transformedVertexTest[4];
        bx::vec4MulMtx(transformedVertexTest, transformedVertex, transformMtx);

        cubeVertices[i] = vector3(transformedVertexTest[0], transformedVertexTest[1], transformedVertexTest[2]);
    }
    
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 1, 5, 4, true, TRIANGLE_MASK_GEOMETRY);
}

void Scene::addAreaLight(bx::Vec3 color, float* transformMtx)
{
    float3 cubeVertices[] = {
        vector3(-0.5f, -0.5f, -0.5f),
        vector3( 0.5f, -0.5f, -0.5f),
        vector3(-0.5f,  0.5f, -0.5f),
        vector3( 0.5f,  0.5f, -0.5f),
        vector3(-0.5f, -0.5f,  0.5f),
        vector3( 0.5f, -0.5f,  0.5f),
        vector3(-0.5f,  0.5f,  0.5f),
        vector3( 0.5f,  0.5f,  0.5f),
    };
    
    for (int i = 0; i < 8; i++) {
        float3 vertex = cubeVertices[i];
        
        float transformedVertex[4] = { vertex.x, vertex.y, vertex.z, 1.0f };
        float transformedVertexTest[4];
        bx::vec4MulMtx(transformedVertexTest, transformedVertex, transformMtx);

        cubeVertices[i] = vector3(transformedVertexTest[0], transformedVertexTest[1], transformedVertexTest[2]);
    }
    
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 1, 5, 4, true, TRIANGLE_MASK_LIGHT);
}

void Scene::createCubeFace(std::vector<float3> & vertices,
                    std::vector<float3> & normals,
                    std::vector<float3> & colors,
                    float3 *cubeVertices,
                    bx::Vec3 color,
                    unsigned int i0,
                    unsigned int i1,
                    unsigned int i2,
                    unsigned int i3,
                    bool inwardNormals,
                    unsigned int triangleMask)
{
    float3 v0 = cubeVertices[i0];
    float3 v1 = cubeVertices[i1];
    float3 v2 = cubeVertices[i2];
    float3 v3 = cubeVertices[i3];
    
    float3 n0 = getTriangleNormal(v0, v1, v2);
    float3 n1 = getTriangleNormal(v0, v2, v3);
    
    if (inwardNormals) {
        n0 = -n0;
        n1 = -n1;
    }
    
    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v0);
    vertices.push_back(v2);
    vertices.push_back(v3);
    
    for (int i = 0; i < 3; i++)
        normals.push_back(n0);
    
    for (int i = 0; i < 3; i++)
        normals.push_back(n1);
    
    for (int i = 0; i < 6; i++)
        colors.push_back(vector3(color.x, color.y, color.z));
    
    for (int i = 0; i < 2; i++)
        masks.push_back(triangleMask);
}
