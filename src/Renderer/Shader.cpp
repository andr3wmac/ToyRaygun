#include "Shader.h"

#include <fstream>
#include <string>
#include <regex>
#include <sstream>
#include <ostream>
#include <iostream>

// Includes skipped by our processing.
std::vector<std::string> Shader::kSkipShaderIncludes
{
    "metal_stdlib",
    "simd/simd.h"
};

std::string Shader::loadAndProcessShader(std::string filename)
{
    std::ifstream shaderFile(filename);
    
    std::smatch match;
    std::regex expr("\\#include\\s*(<([^\"<>|\\b]+)>|\"([^\"<>|\\b]+)\")");
    
    std::stringstream finalShader;
    std::string line;
    while (std::getline(shaderFile, line))
    {
        if (std::regex_search(line, match, expr))
        {
            bool searchIncludeFolders = false;
            
            std::string includeRaw = match.str(1);
            std::string includePath = includeRaw.substr(1, includeRaw.size() - 2);
            
            // Certain includes are handled by the compilers themselves.
            if (std::find(std::begin(kSkipShaderIncludes), std::end(kSkipShaderIncludes), includePath) == std::end(kSkipShaderIncludes))
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
    
    return finalShader.str();
}

bool Shader::Load(const char* path, bool preprocess)
{
    sourcePath = path;
    sourceText.clear();

    std::ifstream shaderFile(path);
    if (shaderFile) 
    {
        sourceText << shaderFile.rdbuf();
        shaderFile.close();
    }
    else 
    {
        return false;
    }

    if (preprocess)
    {
        Preprocess();
    }

    return true;
}

void Shader::Preprocess()
{
    std::smatch match;
    std::regex expr("\\#include\\s*(<([^\"<>|\\b]+)>|\"([^\"<>|\\b]+)\")");

    std::stringstream finalShader;
    std::string line;
    while (std::getline(sourceText, line))
    {
        if (std::regex_search(line, match, expr))
        {
            bool searchIncludeFolders = false;

            std::string includeRaw = match.str(1);
            std::string includePath = includeRaw.substr(1, includeRaw.size() - 2);

            // Certain includes are handled by the compilers themselves.
            if (std::find(std::begin(kSkipShaderIncludes), std::end(kSkipShaderIncludes), includePath) == std::end(kSkipShaderIncludes))
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

    finalShader.seekg(0);

    sourceText.clear();
    sourceText << finalShader.rdbuf();

    std::ofstream outFile;
    outFile.open("test_finalShader.txt");
    outFile << sourceText.rdbuf();
    outFile.close();
}

bool Shader::Compile()
{
    return false;
}

void* Shader::GetBufferPointer()
{
    return nullptr;
}

size_t Shader::GetBufferSize()
{
    return -1;
}