/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#include "Shader.h"

#include <fstream>
#include <string>
#include <regex>
#include <sstream>
#include <ostream>
#include <iostream>

using namespace toyraygun;

// Includes skipped by our processing.
std::vector<std::string> Shader::s_skipShaderIncludes
{
    "metal_stdlib",
    "simd/simd.h",
    "metal_stdlib"
};

bool Shader::load(std::string path, bool doPreprocess)
{
    char fullPath[256];
    snprintf(fullPath, 256, "%s/%s.%s", Platform::getRuntimeShaderPath().c_str(), path.c_str(), Platform::getRuntimeShaderExt().c_str());

    m_sourcePath = fullPath;
    m_sourceText.clear();

    std::ifstream shaderFile(m_sourcePath);
    if (shaderFile) 
    {
        m_sourceText << shaderFile.rdbuf();
        shaderFile.close();
    }
    else 
    {
        return false;
    }

    if (doPreprocess)
    {
        preprocess();
    }

    return true;
}

void Shader::preprocess()
{
    std::smatch match;
    std::regex expr("\\#include\\s*(<([^\"<>|\\b]+)>|\"([^\"<>|\\b]+)\")");

    std::stringstream finalShader;
    std::string line;
    while (std::getline(m_sourceText, line))
    {
        if (std::regex_search(line, match, expr))
        {
            bool searchIncludeFolders = false;

            std::string includeRaw = match.str(1);
            std::string includePath = includeRaw.substr(1, includeRaw.size() - 2);

            // Certain includes are handled by the compilers themselves.
            if (std::find(std::begin(s_skipShaderIncludes), std::end(s_skipShaderIncludes), includePath) == std::end(s_skipShaderIncludes))
            {
                if (includeRaw.front() == '<' && includeRaw.back() == '>')
                {
                    searchIncludeFolders = true;
                }

                std::ifstream includeFile(includePath);
                if (includeFile.good())
                {
                    finalShader << includeFile.rdbuf();
                }
                else
                {
                    std::cout << "Failed to open shader include file: " << includePath << std::endl;
                    finalShader << line << std::endl;
                }
            }
        }
        else
        {
            finalShader << line << std::endl;
        }
    }

    // Copy final shader to source text.
    finalShader.seekg(0);
    m_sourceText.str(finalShader.str());
    
#if _DEBUG
    std::cout << "Processed Shader:" << std::endl;
    std::cout << m_sourceText.str().c_str() << std::endl;
#endif
}

void Shader::addFunction(std::string functionName, ShaderFunctionType functionType)
{
    ShaderFunction shaderFunc;
    shaderFunc.functionName = functionName;
    shaderFunc.functionType = functionType;
    m_functions.push_back(shaderFunc);
}

std::vector<std::string> Shader::getFunctionNames()
{
    std::vector<std::string> functionNames;

    for (int i = 0; i < m_functions.size(); ++i)
    {
        functionNames.push_back(m_functions[i].functionName);
    }

    return functionNames;
}

std::string Shader::getFunction(ShaderFunctionType functionType)
{
    for (int i = 0; i < m_functions.size(); ++i)
    {
        if (m_functions[i].functionType == functionType)
        {
            return m_functions[i].functionName;
        }
    }

    return "";
}

std::wstring Shader::getFunctionW(ShaderFunctionType functionType)
{
    for (int i = 0; i < m_functions.size(); ++i)
    {
        if (m_functions[i].functionType == functionType)
        {
            std::wstring funcNameW = std::wstring(m_functions[i].functionName.begin(), m_functions[i].functionName.end());
            return funcNameW;
        }
    }

    return L"";
}

bool Shader::compile()
{
    return false;
}

std::string Shader::getSourceText()
{
    return m_sourceText.str();
}

void* Shader::getBufferPointer()
{
    return nullptr;
}

size_t Shader::getBufferSize()
{
    return -1;
}
