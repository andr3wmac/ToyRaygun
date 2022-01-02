#pragma once

#include "engine/Shader.h"

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

class D3D12Shader : public toyraygun::Shader
{
protected:
    static CComPtr<IDxcLibrary> m_library;
    static CComPtr<IDxcCompiler> m_compiler;

    CComPtr<IDxcBlobEncoding> m_sourceBlob;
    CComPtr<IDxcBlob> m_compiledBlob;

public:
    D3D12Shader();

    virtual bool compile();

    virtual void* getBufferPointer();
    virtual size_t getBufferSize();
};
