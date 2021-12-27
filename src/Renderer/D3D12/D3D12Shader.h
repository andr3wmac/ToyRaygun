#pragma once

#include "../Shader.h"

#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <memory>

#include <atlbase.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

#include <dxcapi.h>

class D3D12Shader : public Shader
{
protected:
    static CComPtr<IDxcLibrary> library;
    static CComPtr<IDxcCompiler> compiler;

    CComPtr<IDxcBlobEncoding> sourceBlob;
    CComPtr<IDxcBlob> compiledBlob;
    std::string filePath;

public:
    D3D12Shader();

    virtual bool Load(const char* path, bool preprocess = false);
    virtual void Preprocess();
    virtual bool Compile();

    virtual void* GetBufferPointer();
    virtual size_t GetBufferSize();
};
