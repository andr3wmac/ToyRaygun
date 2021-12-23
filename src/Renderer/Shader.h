#ifndef _SHADER_H_
#define _SHADER_H_

#include "Platform.h"
#include <string>

class Shader
{
public:
    static std::vector<std::string> kSkipShaderIncludes;
    static std::string loadAndProcessShader(std::string path);

    virtual void Init(Platform* platform);
    virtual void Destroy();
    
    virtual void RenderFrame();
};

#endif
