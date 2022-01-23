#include "D3D12Shader.h"
#include <iostream>

CComPtr<IDxcLibrary> D3D12Shader::m_library = nullptr;
CComPtr<IDxcCompiler> D3D12Shader::m_compiler = nullptr;

D3D12Shader::D3D12Shader()
{
    if (m_library == nullptr)
    {
        HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library));
        //if(FAILED(hr)) Handle error...
    }

    if (m_compiler == nullptr)
    {
        HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
        //if(FAILED(hr)) Handle error...
    }
}

bool D3D12Shader::compile(ShaderType type)
{
    std::string sourceString = m_sourceText.str();
    HRESULT hr = m_library->CreateBlobWithEncodingOnHeapCopy(sourceString.c_str(), sourceString.length(),
        CP_UTF8, &m_sourceBlob);

    if (FAILED(hr))
    {
        return false;
    }

    if (m_sourceBlob == nullptr)
    {
        OutputDebugString("Shader compilation failed.");
        return false;
    }

    std::wstring sourceName = std::wstring(m_sourcePath.begin(), m_sourcePath.end());
    std::wstring entryPoint = L"";
    std::wstring targetProfile = L"";

    if (type == ShaderType::Compute)
    {
        entryPoint = getFunctionW(ShaderFunctionType::Compute);
        targetProfile = L"cs_6_3";
    }

    if (type == ShaderType::Raytrace)
    {
        entryPoint = L"";
        targetProfile = L"lib_6_3";
    }

    CComPtr<IDxcOperationResult> result;
    hr = m_compiler->Compile(
        m_sourceBlob,          // pSource
        sourceName.c_str(),    // pSourceName
        entryPoint.c_str(),    // pEntryPoint
        targetProfile.c_str(), // pTargetProfile
        NULL, 0,               // pArguments, argCount
        NULL, 0,               // pDefines, defineCount
        NULL,                  // pIncludeHandler
        &result);              // ppResult

    if (SUCCEEDED(hr))
    {
        result->GetStatus(&hr);
    }

    if (FAILED(hr))
    {
        if (result)
        {
            CComPtr<IDxcBlobEncoding> errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if (SUCCEEDED(hr) && errorsBlob)
            {
                OutputDebugString("Shader compilation failed.");
                OutputDebugString((const char*)errorsBlob->GetBufferPointer());
            }
        }

        OutputDebugString("Shader compilation failed.");
        return false;
    }

    result->GetResult(&m_compiledBlob);
    return true;
}

void* D3D12Shader::getBufferPointer()
{
    if (m_compiledBlob == nullptr)
    {
        OutputDebugString("D3D12 SHADER IS NULL.");
        return nullptr;
    }

    return m_compiledBlob->GetBufferPointer();
}

size_t D3D12Shader::getBufferSize()
{
    if (m_compiledBlob == nullptr)
    {
        OutputDebugString("D3D12 SHADER IS NULL.");
        return 0;
    }

    return m_compiledBlob->GetBufferSize();
}

void* D3D12Shader::getCompiledShader()
{
    return m_compiledBlob;
}