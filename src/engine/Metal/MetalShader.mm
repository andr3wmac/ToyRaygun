/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 */

#import "MetalShader.h"
using namespace toyraygun;

#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#import "MetalRenderer.h"

bool MetalShader::compile(ShaderType type)
{
    Engine* engine = Engine::instance();
    
    CAMetalLayer* swapchain = (__bridge CAMetalLayer *)SDL_RenderGetMetalLayer(engine->getRenderer());
    const id<MTLDevice> _device = swapchain.device;
    
    MTLCompileOptions* compileOptions = [MTLCompileOptions new];
    compileOptions.languageVersion = MTLLanguageVersion1_1;
    NSError* compileError;
    
    m_compiledLibrary = [_device newLibraryWithSource:[NSString stringWithCString:getSourceText().c_str()
                                                    encoding:[NSString defaultCStringEncoding]]
                                                    options:compileOptions
                                                    error:&compileError];
                            
    if (compileError != nil)
    {
        NSLog(@" Shader Compile Error => %@ ", [compileError userInfo] );
        m_compiledLibrary = nullptr;
        return false;
    }
    
    return true;
}

void* MetalShader::getCompiledShader()
{
    return m_compiledLibrary;
}
