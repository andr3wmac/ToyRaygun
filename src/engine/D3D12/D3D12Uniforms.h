/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef D3D12_UNIFORMS_HEADER_GUARD
#define D3D12_UNIFORMS_HEADER_GUARD

#include "engine/Platform.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

#include <bx/math.h>

namespace toyraygun
{
    struct UniformFloat3
    {
        DirectX::XMFLOAT3A data;
        
        bx::Vec3 get()
        {
            return bx::Vec3(data.x, data.y, data.z);
        }
        
        void set(bx::Vec3 value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
        }
    };

    struct UniformFloat4x4
    {
        DirectX::XMMATRIX data; //row-major
        
        void get(float* _mtxOut)
        {
            DirectX::XMFLOAT4X4A mat;
            XMStoreFloat4x4A(&mat, data);

            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    _mtxOut[(i * 4) + j] = mat(i, j);
                }
            }
        }
        
        void set(float* _mtxIn)
        {
            DirectX::XMFLOAT4X4A mat;
            
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    mat(i, j) =  _mtxIn[(i * 4) + j];
                }
            }
            
            data = XMLoadFloat4x4A(&mat);
        }
    };
}

#endif // D3D12_UNIFORMS_HEADER_GUARD
