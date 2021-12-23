#ifndef _SHADER_H_
#define _SHADER_H_

#include "../Platform/Platform.h"
#include <string>
#include <vector>

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
