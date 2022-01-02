#ifndef _SHADER_H_
#define _SHADER_H_

#include "Platform/Platform.h"
#include <string>
#include <vector>
#include <sstream>

enum ShaderFunctionType {
    None = 0,
    RayGen,
    ClosestHit,
    Miss,
    Count
};

struct ShaderFunction {
    std::string functionName;
    ShaderFunctionType functionType;
};

class Shader
{
protected:
    std::vector<ShaderFunction> m_functions;

public:
    static std::vector<std::string> kSkipShaderIncludes;
    static std::string loadAndProcessShader(std::string path);
    
    std::string m_sourcePath;
    std::stringstream m_sourceText;

    virtual bool Load(std::string path, bool preprocess = false);
    virtual void Preprocess();
    virtual bool Compile();

    virtual void AddFunction(std::string functionName, ShaderFunctionType functionType = ShaderFunctionType::None);
    virtual std::vector<std::string> GetFunctionNames();
    virtual std::string GetFunction(ShaderFunctionType functionType);
    virtual std::wstring GetFunctionW(ShaderFunctionType functionType);

    virtual void* GetBufferPointer();
    virtual size_t GetBufferSize();
};

#endif
