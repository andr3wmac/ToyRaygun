#include "D3D12Shader.h"

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

bool D3D12Shader::Load(const char* path, bool preprocess)
{
    filePath = path;
    std::wstring sourcePath = std::wstring(filePath.begin(), filePath.end());

    uint32_t codePage = CP_UTF8;
    CComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = library->CreateBlobFromFile(sourcePath.c_str(), &codePage, &sourceBlob);

    return false;
}

void D3D12Shader::Preprocess()
{

}

bool D3D12Shader::Compile()
{
    std::wstring sourceName = std::wstring(filePath.begin(), filePath.end());

    CComPtr<IDxcOperationResult> result;
    HRESULT hr = compiler->Compile(
        sourceBlob, // pSource
        sourceName.c_str(), // pSourceName
        L"main", // pEntryPoint
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
                wprintf(L"Compilation failed with errors:\n%hs\n",
                    (const char*)errorsBlob->GetBufferPointer());
            }
        }

        return false;
    }

    result->GetResult(&compiledBlob);
    return true;
}

void* D3D12Shader::GetBufferPointer()
{
    if (compiledBlob == nullptr)
    {
        return nullptr;
    }

    return compiledBlob->GetBufferPointer();
}

size_t D3D12Shader::GetBufferSize()
{
    if (compiledBlob == nullptr)
    {
        return -1;
    }

    return compiledBlob->GetBufferSize();
}