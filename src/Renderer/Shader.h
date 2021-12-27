#ifndef _SHADER_H_
#define _SHADER_H_

#include "Platform/Platform.h"
#include <string>
#include <vector>

class Shader
{
public:
    static std::vector<std::string> kSkipShaderIncludes;
    static std::string loadAndProcessShader(std::string path);

    virtual bool Load(const char* path, bool preprocess = false);
    virtual void Preprocess();
    virtual bool Compile();

    virtual void* GetBufferPointer();
    virtual size_t GetBufferSize();
};

#endif
