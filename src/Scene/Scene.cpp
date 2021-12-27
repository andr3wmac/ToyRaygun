/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Implementation for scene creation functions
*/


#include "Scene.h"
#include "../Renderer/Renderer.h"
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
    
    addGeometry(verts, tris, 12, transformMtx, color, TRIANGLE_MASK_GEOMETRY);
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
    
    addGeometry(verts, tris, 2, transformMtx, color, TRIANGLE_MASK_GEOMETRY);
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
    
    addGeometry(verts, tris, 2, transformMtx, color, TRIANGLE_MASK_LIGHT);
}

bx::Vec3 applyTransform(bx::Vec3 input, float* transformMtx, float w)
{
    float point[4] = { input.x, input.y, input.z, w };
    float transformedPoint[4];
    bx::vec4MulMtx(transformedPoint, point, transformMtx);
    return bx::Vec3(transformedPoint[0], transformedPoint[1], transformedPoint[2]);
}

void Scene::addGeometry(bx::Vec3* _vertices,
                        uint32_t* _indices,
                        int _triangleCount,
                        float* transformMtx,
                        bx::Vec3 _color,
                        unsigned int _mask)
{
    for (int i = 0; i < _triangleCount; ++i)
    {
        uint32_t idx[] = { _indices[(i * 3) + 0], _indices[(i * 3) + 1], _indices[(i * 3) + 2] };
        bx::Vec3 normal = bx::calcNormal(_vertices[idx[0]], _vertices[idx[1]], _vertices[idx[2]]);
        
        for (int j = 0; j < 3; ++j)
        {
            bx::Vec3 xfrmVert = applyTransform(_vertices[idx[j]], transformMtx, 1.0f);
            bx::Vec3 xfrmNormal = applyTransform(normal, transformMtx, 0.0f);
            
            vertices.push_back(xfrmVert);
            indices.push_back(vertices.size() - 1);
            normals.push_back(xfrmNormal);
            colors.push_back(_color);
        }
        
        // Masks is per-triangle, not per vertex.
        masks.push_back(_mask);
    }
}
