#include "D3D12Shader.h"
#include <iostream>

CComPtr<IDxcLibrary> D3D12Shader::library = nullptr;
CComPtr<IDxcCompiler> D3D12Shader::compiler = nullptr;

D3D12Shader::D3D12Shader()
{
    if (library == nullptr)
    {
        HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
        //if(FAILED(hr)) Handle error...
    }

    if (compiler == nullptr)
    {
        HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        //if(FAILED(hr)) Handle error...
    }
}

bool D3D12Shader::Compile()
{
    std::string sourceString = m_sourceText.str();
    HRESULT hr = library->CreateBlobWithEncodingOnHeapCopy(sourceString.c_str(), sourceString.length(),
        CP_UTF8, &sourceBlob);

    if (FAILED(hr))
    {
        return false;
    }

    if (sourceBlob == nullptr)
    {
        OutputDebugString("Shader compilation failed.");
        return false;
    }

    std::wstring sourceNameWCHAR = std::wstring(m_sourcePath.begin(), m_sourcePath.end());

    CComPtr<IDxcOperationResult> result;
    hr = compiler->Compile(
        sourceBlob, // pSource
        sourceNameWCHAR.c_str(), // pSourceName
        L"", // pEntryPoint
        L"lib_6_3", // pTargetProfile
        NULL, 0, // pArguments, argCount
        NULL, 0, // pDefines, defineCount
        NULL, // pIncludeHandler
        &result); // ppResult

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

    result->GetResult(&compiledBlob);
    return true;
}

void* D3D12Shader::GetBufferPointer()
{
    if (compiledBlob == nullptr)
    {
        OutputDebugString("D3D12 SHADER IS NULL.");
        return nullptr;
    }

    return compiledBlob->GetBufferPointer();
}

size_t D3D12Shader::GetBufferSize()
{
    if (compiledBlob == nullptr)
    {
        OutputDebugString("D3D12 SHADER IS NULL.");
        return 0;
    }

    return compiledBlob->GetBufferSize();
}