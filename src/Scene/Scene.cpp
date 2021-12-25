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
    
    for (int i = 0; i < 8; i++)
    {
        float vertex[4] = { verts[i].x, verts[i].y, verts[i].z, 1.0f };
        float transformedVertex[4];
        bx::vec4MulMtx(transformedVertex, vertex, transformMtx);

        verts[i] = bx::Vec3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
    }
    
    addQuad(verts, color, 0, 4, 6, 2, false, TRIANGLE_MASK_GEOMETRY);
    addQuad(verts, color, 1, 3, 7, 5, false, TRIANGLE_MASK_GEOMETRY);
    addQuad(verts, color, 0, 1, 5, 4, false, TRIANGLE_MASK_GEOMETRY);
    addQuad(verts, color, 2, 6, 7, 3, false, TRIANGLE_MASK_GEOMETRY);
    addQuad(verts, color, 0, 2, 3, 1, false, TRIANGLE_MASK_GEOMETRY);
    addQuad(verts, color, 4, 5, 7, 6, false, TRIANGLE_MASK_GEOMETRY);
}

void Scene::addPlane(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 verts[] = {
        cubeVertices[0],
        cubeVertices[1],
        cubeVertices[5],
        cubeVertices[4]
    };
    
    for (int i = 0; i < 4; i++)
    {
        float vertex[4] = { verts[i].x, verts[i].y, verts[i].z, 1.0f };
        float transformedVertex[4];
        bx::vec4MulMtx(transformedVertex, vertex, transformMtx);

        verts[i] = bx::Vec3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
    }
    
    addQuad(verts, color, 0, 1, 2, 3, true, TRIANGLE_MASK_GEOMETRY);
}

void Scene::addAreaLight(bx::Vec3 color, float* transformMtx)
{
    bx::Vec3 verts[] = {
        cubeVertices[0],
        cubeVertices[1],
        cubeVertices[5],
        cubeVertices[4]
    };
    
    for (int i = 0; i < 4; i++)
    {
        float vertex[4] = { verts[i].x, verts[i].y, verts[i].z, 1.0f };
        float transformedVertex[4];
        bx::vec4MulMtx(transformedVertex, vertex, transformMtx);

        verts[i] = bx::Vec3(transformedVertex[0], transformedVertex[1], transformedVertex[2]);
    }
    
    addQuad(verts, color, 0, 1, 2, 3, true, TRIANGLE_MASK_LIGHT);
}

void Scene::addQuad(bx::Vec3* quadVertices,
                    bx::Vec3 color,
                    unsigned int i0,
                    unsigned int i1,
                    unsigned int i2,
                    unsigned int i3,
                    bool inwardNormals,
                    unsigned int triangleMask)
{
    bx::Vec3 normal0 = getTriangleNormal(quadVertices[i0], quadVertices[i1], quadVertices[i2]);
    bx::Vec3 normal1 = getTriangleNormal(quadVertices[i0], quadVertices[i2], quadVertices[i3]);
    
    if (inwardNormals) {
        normal0 = bx::neg(normal0);
        normal1 = bx::neg(normal1);
    }
    
    vertices.push_back(quadVertices[i0]);
    vertices.push_back(quadVertices[i1]);
    vertices.push_back(quadVertices[i2]);
    vertices.push_back(quadVertices[i0]);
    vertices.push_back(quadVertices[i2]);
    vertices.push_back(quadVertices[i3]);
    
    for (int i = 0; i < 3; i++)
        normals.push_back(normal0);
    
    for (int i = 0; i < 3; i++)
        normals.push_back(normal1);
    
    for (int i = 0; i < 6; i++)
        colors.push_back(color);
    
    for (int i = 0; i < 2; i++)
        masks.push_back(triangleMask);
}
