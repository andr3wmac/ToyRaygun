/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef SHADER_HEADER_GUARD
#define SHADER_HEADER_GUARD

#include "platform/Platform.h"
#include <string>
#include <vector>
#include <sstream>

namespace toyraygun
{
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
        static std::vector<std::string> s_skipShaderIncludes;

        std::string m_sourcePath;
        std::stringstream m_sourceText;

        virtual bool load(std::string path, bool doPreprocess = false);
        virtual void preprocess();
        virtual bool compile();

        virtual void addFunction(std::string functionName, ShaderFunctionType functionType = ShaderFunctionType::None);
        virtual std::vector<std::string> getFunctionNames();
        virtual std::string getFunction(ShaderFunctionType functionType);
        virtual std::wstring getFunctionW(ShaderFunctionType functionType);

        virtual std::string getSourceText();
        virtual void* getBufferPointer();
        virtual size_t getBufferSize();
    };
}

#endif // SHADER_HEADER_GUARD