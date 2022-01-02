/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Scene.h"
#include "engine/Renderer.h"
using namespace toyraygun;

#include <iostream>
#include <bx/math.h>

static bx::Vec3 cubeVertices[] = {
    bx::Vec3(-0.5f, -0.5f, -0.5f),
    bx::Vec3( 0.5f, -0.5f, -0.5f),
    bx::Vec3(-0.5f,  0.5f, -0.5f),
    bx::Vec3( 0.5f,  0.5f, -0.5f),
    bx::Vec3(-0.5f, -0.5f,  0.5f),
    bx::Vec3( 0.5f, -0.5f,  0.5f),
    bx::Vec3(-0.5f,  0.5f,  0.5f),
    bx::Vec3( 0.5f,  0.5f,  0.5f),
};

void Scene::addCube(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 verts[] = {
        cubeVertices[0],
        cubeVertices[1],
        cubeVertices[2],
        cubeVertices[3],
        cubeVertices[4],
        cubeVertices[5],
        cubeVertices[6],
        cubeVertices[7]
    };
    
    uint32_t tris[] = {
        0, 4, 6,
        0, 6, 2,
        
        1, 3, 7,
        1, 7, 5,
        
        0, 1, 5,
        0, 5, 4,
        
        2, 6, 7,
        2, 7, 3,
        
        0, 2, 3,
        0, 3, 1,
        
        4, 5, 7,
        4, 7, 6
    };
    
    addGeometry(verts, tris, 12, transformMtx, color, MATERIAL_DEFAULT);
}

void Scene::addPlane(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 verts[] = {
        cubeVertices[0],
        cubeVertices[1],
        cubeVertices[5],
        cubeVertices[4]
    };
    
    uint32_t tris[] = {
        0, 2, 1,
        0, 3, 2,
    };
    
    addGeometry(verts, tris, 2, transformMtx, color, MATERIAL_DEFAULT);
}

void Scene::addAreaLight(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 verts[] = {
        cubeVertices[0],
        cubeVertices[1],
        cubeVertices[5],
        cubeVertices[4]
    };
    
    uint32_t tris[] = {
        0, 2, 1,
        0, 3, 2,
    };
    
    addGeometry(verts, tris, 2, transformMtx, color, MATERIAL_EMISSIVE);
}

bx::Vec3 applyTransform(bx::Vec3 input, float* transformMtx, float w)
{
    float point[4] = { input.x, input.y, input.z, w };
    float transformedPoint[4];
    bx::vec4MulMtx(transformedPoint, point, transformMtx);
    return bx::Vec3(transformedPoint[0], transformedPoint[1], transformedPoint[2]);
}

void Scene::addGeometry(bx::Vec3* vertices,
                        uint32_t* indices,
                        int triangleCount,
                        float* transformMtx,
                        bx::Vec3 color,
                        unsigned int materialID)
{
    for (int i = 0; i < triangleCount; ++i)
    {
        uint32_t idx[] = { indices[(i * 3) + 0], indices[(i * 3) + 1], indices[(i * 3) + 2] };
        bx::Vec3 normal = bx::calcNormal(vertices[idx[0]], vertices[idx[1]], vertices[idx[2]]);
        
        for (int j = 0; j < 3; ++j)
        {
            bx::Vec3 xfrmVert = applyTransform(vertices[idx[j]], transformMtx, 1.0f);
            bx::Vec3 xfrmNormal = applyTransform(normal, transformMtx, 0.0f);
            xfrmNormal = bx::normalize(xfrmNormal);

            m_vertexBuffer.push_back(xfrmVert);
            m_indexBuffer.push_back(m_vertexBuffer.size() - 1);
            m_normalBuffer.push_back(xfrmNormal);
            m_colorBuffer.push_back(color);
        }
        
        // Materials are per-triangle, not per vertex.
        m_materialIDBuffer.push_back(materialID);
    }
}
