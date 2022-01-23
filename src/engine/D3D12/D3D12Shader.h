#pragma once

#include "engine/Shader.h"

#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include <atlbase.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;
#include <dxcapi.h>

using namespace toyraygun;

class D3D12Shader : public toyraygun::Shader
{
protected:
    static CComPtr<IDxcLibrary> m_library;
    static CComPtr<IDxcCompiler> m_compiler;

    CComPtr<IDxcBlobEncoding> m_sourceBlob;
    std::vector<CComPtr<IDxcBlob>> m_compiledBlobs;

    bool compileBlob(int blobIndex, std::wstring sourceName, std::wstring entryPoint, std::wstring targetProfile);

public:
    D3D12Shader();

    virtual bool compile(ShaderType type);

    virtual void* getBufferPointer(ShaderFunctionType type = ShaderFunctionType::None);
    virtual size_t getBufferSize(ShaderFunctionType type = ShaderFunctionType::None);
    virtual void* getCompiledShader(ShaderFunctionType type = ShaderFunctionType::None);
};
