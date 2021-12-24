/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Implementation for scene creation functions
*/


#include "Scene.h"
#include "../Renderer/ShaderTypes.h"

#include <bx/math.h>

bx::Vec3 getTriangleNormal(bx::Vec3 v0, bx::Vec3 v1, bx::Vec3 v2) {
    bx::Vec3 e1 = bx::normalize(bx::sub(v1, v0));
    bx::Vec3 e2 = bx::normalize(bx::sub(v2, v0));

    return bx::cross(e1, e2);
}

void Scene::addCube(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 cubeVertices[] = {
        bx::Vec3(-0.5f, -0.5f, -0.5f),
        bx::Vec3( 0.5f, -0.5f, -0.5f),
        bx::Vec3(-0.5f,  0.5f, -0.5f),
        bx::Vec3( 0.5f,  0.5f, -0.5f),
        bx::Vec3(-0.5f, -0.5f,  0.5f),
        bx::Vec3( 0.5f, -0.5f,  0.5f),
        bx::Vec3(-0.5f,  0.5f,  0.5f),
        bx::Vec3( 0.5f,  0.5f,  0.5f),
    };
    
    for (int i = 0; i < 8; i++) {
        bx::Vec3 vertex = cubeVertices[i];

        float transformedVertex[4] = { vertex.x, vertex.y, vertex.z, 1.0f };
        float transformedVertexTest[4];
        bx::vec4MulMtx(transformedVertexTest, transformedVertex, transformMtx);

        cubeVertices[i] = bx::Vec3(transformedVertexTest[0], transformedVertexTest[1], transformedVertexTest[2]);
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
    bx::Vec3 cubeVertices[] = {
        bx::Vec3(-0.5f, -0.5f, -0.5f),
        bx::Vec3( 0.5f, -0.5f, -0.5f),
        bx::Vec3(-0.5f,  0.5f, -0.5f),
        bx::Vec3( 0.5f,  0.5f, -0.5f),
        bx::Vec3(-0.5f, -0.5f,  0.5f),
        bx::Vec3( 0.5f, -0.5f,  0.5f),
        bx::Vec3(-0.5f,  0.5f,  0.5f),
        bx::Vec3( 0.5f,  0.5f,  0.5f),
    };
    
    for (int i = 0; i < 8; i++) {
        bx::Vec3 vertex = cubeVertices[i];

        float transformedVertex[4] = { vertex.x, vertex.y, vertex.z, 1.0f };
        bx::vec4MulMtx(transformedVertex, transformedVertex, transformMtx);

        cubeVertices[i] = bx::Vec3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
    }
    
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 1, 5, 4, true, TRIANGLE_MASK_GEOMETRY);
}

void Scene::addAreaLight(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 cubeVertices[] = {
        bx::Vec3(-0.5f, -0.5f, -0.5f),
        bx::Vec3( 0.5f, -0.5f, -0.5f),
        bx::Vec3(-0.5f,  0.5f, -0.5f),
        bx::Vec3( 0.5f,  0.5f, -0.5f),
        bx::Vec3(-0.5f, -0.5f,  0.5f),
        bx::Vec3( 0.5f, -0.5f,  0.5f),
        bx::Vec3(-0.5f,  0.5f,  0.5f),
        bx::Vec3( 0.5f,  0.5f,  0.5f),
    };
    
    for (int i = 0; i < 8; i++) {
        bx::Vec3 vertex = cubeVertices[i];
        
        float transformedVertex[4] = { vertex.x, vertex.y, vertex.z, 1.0f };
        bx::vec4MulMtx(transformedVertex, transformedVertex, transformMtx);
        
        cubeVertices[i] = bx::Vec3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
    }
    
    createCubeFace(vertices, normals, colors, cubeVertices, color, 0, 1, 5, 4, true, TRIANGLE_MASK_LIGHT);
}

void Scene::createCubeFace(std::vector<bx::Vec3>& vertices,
                            std::vector<bx::Vec3>& normals,
                            std::vector<bx::Vec3>& colors,
                            bx::Vec3* cubeVertices,
                            bx::Vec3 color,
                            unsigned int i0,
                            unsigned int i1,
                            unsigned int i2,
                            unsigned int i3,
                            bool inwardNormals,
                            unsigned int triangleMask)
{
    bx::Vec3 v0 = cubeVertices[i0];
    bx::Vec3 v1 = cubeVertices[i1];
    bx::Vec3 v2 = cubeVertices[i2];
    bx::Vec3 v3 = cubeVertices[i3];
    
    bx::Vec3 n0 = getTriangleNormal(v0, v1, v2);
    bx::Vec3 n1 = getTriangleNormal(v0, v2, v3);
    
    if (inwardNormals) {
        n0 = bx::neg(n0);
        n1 = bx::neg(n1);
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
        colors.push_back(color);
    
    for (int i = 0; i < 2; i++)
        masks.push_back(triangleMask);
}
