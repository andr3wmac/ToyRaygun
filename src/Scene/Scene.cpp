/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Implementation for scene creation functions
*/


#include "Scene.h"
#include "ShaderTypes.h"
#include <iostream>
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
    
    for (int i = 0; i < 8; i++)
    {
        float vertex[4] = { cubeVertices[i].x, cubeVertices[i].y, cubeVertices[i].z, 1.0f };
        float transformedVertex[4];
        bx::vec4MulMtx(transformedVertex, vertex, transformMtx);

        cubeVertices[i] = bx::Vec3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
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
    
    for (int i = 0; i < 8; i++)
    {
        float vertex[4] = { cubeVertices[i].x, cubeVertices[i].y, cubeVertices[i].z, 1.0f };
        float transformedVertex[4];
        bx::vec4MulMtx(transformedVertex, vertex, transformMtx);

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
    
    for (int i = 0; i < 8; i++)
    {
        float vertex[4] = { cubeVertices[i].x, cubeVertices[i].y, cubeVertices[i].z, 1.0f };
        float transformedVertex[4];
        bx::vec4MulMtx(transformedVertex, vertex, transformMtx);

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
    bx::Vec3 normal0 = getTriangleNormal(cubeVertices[i0], cubeVertices[i1], cubeVertices[i2]);
    bx::Vec3 normal1 = getTriangleNormal(cubeVertices[i0], cubeVertices[i2], cubeVertices[i3]);
    
    if (inwardNormals) {
        normal0 = bx::neg(normal0);
        normal1 = bx::neg(normal1);
    }
    
    vertices.push_back(cubeVertices[i0]);
    vertices.push_back(cubeVertices[i1]);
    vertices.push_back(cubeVertices[i2]);
    vertices.push_back(cubeVertices[i0]);
    vertices.push_back(cubeVertices[i2]);
    vertices.push_back(cubeVertices[i3]);
    
    for (int i = 0; i < 3; i++)
        normals.push_back(normal0);
    
    for (int i = 0; i < 3; i++)
        normals.push_back(normal1);
    
    for (int i = 0; i < 6; i++)
        colors.push_back(color);
    
    for (int i = 0; i < 2; i++)
        masks.push_back(triangleMask);
}
