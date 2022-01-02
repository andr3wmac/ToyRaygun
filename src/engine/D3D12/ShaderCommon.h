//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

namespace shadercommon
{
    using namespace DirectX;

    // Shader will use byte encoding to access indices.
    typedef UINT16 Index;

    struct AreaLight
    {
        XMVECTOR position;
        XMVECTOR forward;
        XMVECTOR right;
        XMVECTOR up;
        XMVECTOR color;
    };

    struct CubeConstantBuffer
    {
        XMFLOAT4 albedo;
    };

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT3 color;
    };

    struct SceneConstantBuffer
    {
        XMMATRIX projectionToWorld; //row-major
        XMVECTOR cameraPosition;
        AreaLight light;
    };

}

#endif // RAYTRACINGHLSLCOMPAT_H