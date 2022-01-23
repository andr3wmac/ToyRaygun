/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#ifndef SHADER_HEADER_GUARD
#define SHADER_HEADER_GUARD

#include "engine/Engine.h"
#include <string>
#include <vector>
#include <sstream>

namespace toyraygun
{
    enum class ShaderType {
        None = 0,
        Graphics,
        Compute,
        Raytrace,
        Count
    };

    enum class ShaderFunctionType {
        None = 0,

        // Graphics Types
        Vertex,
        Fragment,
        
        // Compute Types
        Compute,

        // Raytracing Types
        RayGen,
        ClosestHit,
        Miss,
        ShadowHit,
        ShadowMiss,

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

        std::string m_path;
        std::string m_sourcePath;
        std::stringstream m_sourceText;

        virtual bool load(std::string path, bool preprocess = true);
        virtual void preprocess();
        virtual bool compile(ShaderType type);

        virtual void addFunction(std::string functionName, ShaderFunctionType functionType = ShaderFunctionType::None);
        virtual std::vector<std::string> getFunctionNames();
        virtual std::string getFunction(ShaderFunctionType functionType);
        virtual std::wstring getFunctionW(ShaderFunctionType functionType);

        virtual std::string getSourceText();
        virtual void* getBufferPointer(ShaderFunctionType type = ShaderFunctionType::None);
        virtual size_t getBufferSize(ShaderFunctionType type = ShaderFunctionType::None);
        virtual void* getCompiledShader(ShaderFunctionType type = ShaderFunctionType::None);
    };
}

#endif // SHADER_HEADER_GUARD
