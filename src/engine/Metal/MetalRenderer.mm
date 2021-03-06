/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE
 
 * This renderer is based on Metal for Accelerating Ray Tracing from:
   https://developer.apple.com/documentation/metalperformanceshaders/metal_for_accelerating_ray_tracing
 * By Apple
 */


#import <simd/simd.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "MetalRenderer.h"
#include "engine/Renderer.h"
#include "engine/Scene.h"
#include "engine/Shader.h"
#include "engine/Texture.h"

#include <fstream>
#include <string>
#include <regex>
#include <sstream>
#include <iostream>

#include <SDL.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

using namespace simd;
using namespace toyraygun;

static const NSUInteger maxFramesInFlight = 3;
static const size_t alignedUniformsSize = (sizeof(toyraygun::Uniforms) + 255) & ~255;
static const size_t rayStride = 48;
static const size_t intersectionStride = sizeof(MPSIntersectionDistancePrimitiveIndexCoordinates);

@interface _MetalRenderer : NSObject

-(nonnull instancetype)initWithDevice:(nonnull id<MTLDevice>)device
                       parentRenderer:(toyraygun::MetalRenderer*)parent;
- (void)resize:(CGSize)size;

- (void)render:(nonnull id<CAMetalDrawable>)surface;

@end

@implementation _MetalRenderer
{
    id <MTLDevice> _device;
    id <MTLCommandQueue> _queue;
    id <MTLLibrary> _rtLib;
    id <MTLLibrary> _accumulateLib;
    id <MTLLibrary> _postProcessingLib;
    
    MPSTriangleAccelerationStructure *_accelerationStructure;
    MPSRayIntersector *_intersector;
    
    id <MTLBuffer> _vertexPositionBuffer;
    id <MTLBuffer> _indexBuffer;
    id <MTLBuffer> _vertexNormalBuffer;
    id <MTLBuffer> _vertexColorBuffer;
    id <MTLBuffer> _rayBuffer;
    id <MTLBuffer> _shadowRayBuffer;
    id <MTLBuffer> _intersectionBuffer;
    id <MTLBuffer> _uniformBuffer;
    id <MTLBuffer> _triangleMaskBuffer;
    
    id <MTLComputePipelineState> _rayPipeline;
    id <MTLComputePipelineState> _shadePipeline;
    id <MTLComputePipelineState> _shadowPipeline;
    id <MTLComputePipelineState> _accumulatePipeline;
    id <MTLRenderPipelineState> _copyPipeline;
    
    id <MTLTexture> _renderTargets[2];
    id <MTLTexture> _accumulationTargets[2];
    id <MTLTexture> _randomTexture;
    
    dispatch_semaphore_t _sem;
    CGSize _size;
    NSUInteger _uniformBufferOffset;
    NSUInteger _uniformBufferIndex;

    unsigned int _frameIndex;
    
@public
    toyraygun::MetalRenderer* _parent;
}

-(nonnull instancetype)initWithDevice:(nonnull id<MTLDevice>)device
                       parentRenderer:(toyraygun::MetalRenderer*)parent;
{
    self = [super init];

    if (self)
    {
        _parent = parent;
        _device = device;
        
        NSLog(@"Metal device: %@", _device.name);
    }

    return self;
}

- (void)createPipelines
{
    NSError *error = NULL;
 
    _queue = [_device newCommandQueue];
    
    // Create compute pipelines will will execute code on the GPU
    MTLComputePipelineDescriptor *computeDescriptor = [[MTLComputePipelineDescriptor alloc] init];

    // Set to YES to allow compiler to make certain optimizations
    computeDescriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;
    
    // Raytracing
    toyraygun::Shader* rtShader = _parent->getShader("Raytracing");
    _rtLib = (id<MTLLibrary>)rtShader->getCompiledShader();
    
    NSString* rayGenFnName = [NSString stringWithCString:rtShader->getFunction(ShaderFunctionType::RayGen).c_str()
                                                encoding:[NSString defaultCStringEncoding]];
    
    // Generates rays according to view/projection matrices
    computeDescriptor.computeFunction = [_rtLib newFunctionWithName:rayGenFnName];
    
    _rayPipeline = [_device newComputePipelineStateWithDescriptor:computeDescriptor
                                                          options:0
                                                       reflection:nil
                                                            error:&error];
    
    if (!_rayPipeline)
        NSLog(@"Failed to create pipeline state: %@", error);
        
    NSString* closestHitFnName = [NSString stringWithCString:rtShader->getFunction(ShaderFunctionType::ClosestHit).c_str()
                                                encoding:[NSString defaultCStringEncoding]];
    
    // Consumes ray/scene intersection test results to perform shading
    computeDescriptor.computeFunction = [_rtLib newFunctionWithName:closestHitFnName];
    
    _shadePipeline = [_device newComputePipelineStateWithDescriptor:computeDescriptor
                                                          options:0
                                                       reflection:nil
                                                            error:&error];
    
    if (!_shadePipeline)
        NSLog(@"Failed to create pipeline state: %@", error);
    
    NSString* shadowFnName = [NSString stringWithCString:rtShader->getFunction(ShaderFunctionType::ShadowHit).c_str()
                                                encoding:[NSString defaultCStringEncoding]];
    
    // Consumes shadow ray intersection tests to update the output image
    computeDescriptor.computeFunction = [_rtLib newFunctionWithName:shadowFnName];
    
    _shadowPipeline = [_device newComputePipelineStateWithDescriptor:computeDescriptor
                                                             options:0
                                                          reflection:nil
                                                               error:&error];
    
    if (!_shadowPipeline)
        NSLog(@"Failed to create pipeline state: %@", error);
    
    // Accumulate Pipeline
    toyraygun::Shader* accumulateShader = _parent->getShader("Accumulate");
    _accumulateLib = (id<MTLLibrary>)accumulateShader->getCompiledShader();

    NSString* accumulateFnName = [NSString stringWithCString:accumulateShader->getFunction(ShaderFunctionType::Compute).c_str()
                                                    encoding:[NSString defaultCStringEncoding]];
    
    computeDescriptor.computeFunction = [_accumulateLib newFunctionWithName:accumulateFnName];
    
    _accumulatePipeline = [_device newComputePipelineStateWithDescriptor:computeDescriptor
                                                                 options:0
                                                              reflection:nil
                                                                   error:&error];
    
    if (!_accumulatePipeline)
        NSLog(@"Failed to create pipeline state: %@", error);
    
    // Post Processing Pipeline
    toyraygun::Shader* postProcessingShader = _parent->getShader("PostProcessing");
    _postProcessingLib = (id<MTLLibrary>)postProcessingShader->getCompiledShader();
    
    NSString* vertexFnName = [NSString stringWithCString:postProcessingShader->getFunction(ShaderFunctionType::Vertex).c_str()
                                                    encoding:[NSString defaultCStringEncoding]];
    
    NSString* fragmentFnName = [NSString stringWithCString:postProcessingShader->getFunction(ShaderFunctionType::Fragment).c_str()
                                                    encoding:[NSString defaultCStringEncoding]];

    // Copies rendered scene into the MTKView
    MTLRenderPipelineDescriptor *renderDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    renderDescriptor.sampleCount = 1;
    renderDescriptor.vertexFunction = [_postProcessingLib newFunctionWithName:vertexFnName];
    renderDescriptor.fragmentFunction = [_postProcessingLib newFunctionWithName:fragmentFnName];
    renderDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    _copyPipeline = [_device newRenderPipelineStateWithDescriptor:renderDescriptor error:&error];
    
    if (!_copyPipeline)
        NSLog(@"Failed to create pipeline state, error %@", error);
}

- (void)loadScene:(toyraygun::Scene*)scene
{
    _sem = dispatch_semaphore_create(maxFramesInFlight);
    
    [self createPipelines];
    
    // Uniform buffer contains a few small values which change from frame to frame. We will have up to 3
    // frames in flight at once, so allocate a range of the buffer for each frame. The GPU will read from
    // one chunk while the CPU writes to the next chunk. Each chunk must be aligned to 256 bytes on macOS
    // and 16 bytes on iOS.
    NSUInteger uniformBufferSize = alignedUniformsSize * maxFramesInFlight;
    
    // Vertex data should be stored in private or managed buffers on discrete GPU systems (AMD, NVIDIA).
    // Private buffers are stored entirely in GPU memory and cannot be accessed by the CPU. Managed
    // buffers maintain a copy in CPU memory and a copy in GPU memory.
    MTLResourceOptions options = 0;

#if !TARGET_OS_IPHONE
    options = MTLResourceStorageModeManaged;
#else
    options = MTLResourceStorageModeShared;
#endif
    
    _uniformBuffer = [_device newBufferWithLength:uniformBufferSize options:options];

    // Convert 12-byte vec3s to 16-byte float3s for vertex positions.
    float3* vertices = new float3[scene->m_vertexBuffer.size()];
    for (int i = 0; i < scene->m_vertexBuffer.size(); ++i)
    {
        vertices[i] = { scene->m_vertexBuffer[i].x, scene->m_vertexBuffer[i].y, scene->m_vertexBuffer[i].z };
    }
    
    // Allocate buffers for vertex positions, colors, and normals. Note that each vertex position is a
    // float3, which is a 16 byte aligned type.
    _vertexPositionBuffer = [_device newBufferWithLength:scene->m_vertexBuffer.size() * sizeof(float3) options:options];
    _indexBuffer = [_device newBufferWithLength:scene->m_indexBuffer.size() * sizeof(uint32_t) options:options];
    _vertexColorBuffer = [_device newBufferWithLength:scene->m_colorBuffer.size() * sizeof(bx::Vec3) options:options];
    _vertexNormalBuffer = [_device newBufferWithLength:scene->m_normalBuffer.size() * sizeof(bx::Vec3) options:options];
    _triangleMaskBuffer = [_device newBufferWithLength:scene->m_materialIDBuffer.size() * sizeof(uint32_t) options:options];
    
    // Copy vertex data into buffers
    memcpy(_vertexPositionBuffer.contents, &vertices[0], _vertexPositionBuffer.length);
    memcpy(_indexBuffer.contents, &scene->m_indexBuffer[0], _indexBuffer.length);
    memcpy(_vertexColorBuffer.contents, &scene->m_colorBuffer[0], _vertexColorBuffer.length);
    memcpy(_vertexNormalBuffer.contents, &scene->m_normalBuffer[0], _vertexNormalBuffer.length);
    memcpy(_triangleMaskBuffer.contents, &scene->m_materialIDBuffer[0], _triangleMaskBuffer.length);;
    
    // Cleanup.
    delete[] vertices;
    
    // When using managed buffers, we need to indicate that we modified the buffer so that the GPU
    // copy can be updated
#if !TARGET_OS_IPHONE
    [_vertexPositionBuffer didModifyRange:NSMakeRange(0, _vertexPositionBuffer.length)];
    [_indexBuffer didModifyRange:NSMakeRange(0, _indexBuffer.length)];
    [_vertexColorBuffer didModifyRange:NSMakeRange(0, _vertexColorBuffer.length)];
    [_vertexNormalBuffer didModifyRange:NSMakeRange(0, _vertexNormalBuffer.length)];
    [_triangleMaskBuffer didModifyRange:NSMakeRange(0, _triangleMaskBuffer.length)];
#endif
    
    // Create a raytracer for our Metal device
    _intersector = [[MPSRayIntersector alloc] initWithDevice:_device];
    
    _intersector.rayDataType = MPSRayDataTypeOriginMaskDirectionMaxDistance;
    _intersector.rayStride = rayStride;
    _intersector.rayMaskOptions = MPSRayMaskOptionPrimitive;
    
    // Create an acceleration structure from our vertex position data
    _accelerationStructure = [[MPSTriangleAccelerationStructure alloc] initWithDevice:_device];
    
    _accelerationStructure.vertexBuffer = _vertexPositionBuffer;
    _accelerationStructure.indexBuffer = _indexBuffer;
    _accelerationStructure.maskBuffer = _triangleMaskBuffer;
    _accelerationStructure.triangleCount = scene->m_indexBuffer.size() / 3;
    
    [_accelerationStructure rebuild];
}

- (void)resize:(CGSize)size
{
    _size = size;
    
    // Handle window size changes by allocating a buffer large enough to contain one standard ray,
    // one shadow ray, and one ray/triangle intersection result per pixel
    NSUInteger rayCount = (NSUInteger)_size.width * (NSUInteger)_size.height;
    
    // We use private buffers here because rays and intersection results will be entirely produced
    // and consumed on the GPU
    _rayBuffer = [_device newBufferWithLength:rayStride * rayCount options:MTLResourceStorageModePrivate];
    _shadowRayBuffer = [_device newBufferWithLength:rayStride * rayCount options:MTLResourceStorageModePrivate];
    _intersectionBuffer = [_device newBufferWithLength:intersectionStride * rayCount options:MTLResourceStorageModePrivate];
    
    // Create a render target which the shading kernel can write to
    MTLTextureDescriptor *renderTargetDescriptor = [[MTLTextureDescriptor alloc] init];
    
    renderTargetDescriptor.pixelFormat = MTLPixelFormatRGBA32Float;
    renderTargetDescriptor.textureType = MTLTextureType2D;
    renderTargetDescriptor.width = size.width;
    renderTargetDescriptor.height = size.height;

    // Stored in private memory because it will only be read and written from the GPU
    renderTargetDescriptor.storageMode = MTLStorageModePrivate;

    // Indicate that we will read and write the texture from the GPU
    renderTargetDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    
    for (NSUInteger i = 0; i < 2; i++) {
        _renderTargets[i] = [_device newTextureWithDescriptor:renderTargetDescriptor];
        _accumulationTargets[i] = [_device newTextureWithDescriptor:renderTargetDescriptor];
    }
    
    renderTargetDescriptor.pixelFormat = MTLPixelFormatR32Uint;
    renderTargetDescriptor.usage = MTLTextureUsageShaderRead;
#if !TARGET_OS_IPHONE
    renderTargetDescriptor.storageMode = MTLStorageModeManaged;
#else
    renderTargetDescriptor.storageMode = MTLStorageModeShared;
#endif
    
    // Generate a texture containing a random integer value for each pixel. This value
    // will be used to decorrelate pixels while drawing pseudorandom numbers from the
    // Halton sequence.
    _randomTexture = [_device newTextureWithDescriptor:renderTargetDescriptor];
    
    toyraygun::Texture randomTex = toyraygun::Texture::generateRandomTexture(size.width, size.height, 4);
    
    [_randomTexture replaceRegion:MTLRegionMake2D(0, 0, size.width, size.height)
                      mipmapLevel:0
                        withBytes:randomTex.getBufferPointer()
                      bytesPerRow:sizeof(uint32_t) * size.width];
    
    randomTex.destroy();
    
    _frameIndex = 0;
}

- (void)updateUniforms {
    // Update this frame's uniforms
    _uniformBufferOffset = alignedUniformsSize * _uniformBufferIndex;

    toyraygun::Uniforms *uniforms = (toyraygun::Uniforms *)((char *)_uniformBuffer.contents + _uniformBufferOffset);
    
    float viewProjMtx[16];
    _parent->getViewProjMtx(viewProjMtx);
    
    float invViewProjMtx[16];
    bx::mtxInverse(invViewProjMtx, viewProjMtx);
    
    uniforms->camera.position.set(_parent->getCameraPosition());
    uniforms->camera.invViewProjMtx.set(invViewProjMtx);
    
    uniforms->light.position.set(bx::Vec3(0.0f, 1.98f, 0.0f));
    uniforms->light.forward.set(bx::Vec3(0.0f, -1.0f, 0.0f));
    uniforms->light.right.set(bx::Vec3(0.25f, 0.0f, 0.0f));
    uniforms->light.up.set(bx::Vec3(0.0f, 0.0f, 0.25f));
    uniforms->light.color.set(bx::Vec3(1.0f, 1.0f, 1.0f));
    
    uniforms->width = (unsigned int)_size.width;
    uniforms->height = (unsigned int)_size.height;
    uniforms->frameIndex = _frameIndex++;
    
#if !TARGET_OS_IPHONE
    [_uniformBuffer didModifyRange:NSMakeRange(_uniformBufferOffset, alignedUniformsSize)];
#endif
    
    // Advance to the next slot in the uniform buffer
    _uniformBufferIndex = (_uniformBufferIndex + 1) % maxFramesInFlight;
}

- (void)render:(nonnull id<CAMetalDrawable>)surface
{
    // We are using the uniform buffer to stream uniform data to the GPU, so we need to wait until the oldest
    // GPU frame has completed before we can reuse that space in the buffer.
    dispatch_semaphore_wait(_sem, DISPATCH_TIME_FOREVER);

    // Create a command buffer which will contain our GPU commands
    id <MTLCommandBuffer> commandBuffer = [_queue commandBuffer];

    // When the frame has finished, signal that we can reuse the uniform buffer space from this frame.
    // Note that the contents of completion handlers should be as fast as possible as the GPU driver may
    // have other work scheduled on the underlying dispatch queue.
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(self->_sem);
    }];

    [self updateUniforms];
    
    NSUInteger width = (NSUInteger)_size.width;
    NSUInteger height = (NSUInteger)_size.height;
    
    // We will launch a rectangular grid of threads on the GPU to generate the rays. Threads are launched in
    // groups called "threadgroups". We need to align the number of threads to be a multiple of the threadgroup
    // size. We indicated when compiling the pipeline that the threadgroup size would be a multiple of the thread
    // execution width (SIMD group size) which is typically 32 or 64 so 8x8 is a safe threadgroup size which
    // should be small to be supported on most devices. A more advanced application would choose the threadgroup
    // size dynamically.
    MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 1);
    MTLSize threadgroups = MTLSizeMake((width  + threadsPerThreadgroup.width  - 1) / threadsPerThreadgroup.width,
                                       (height + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height,
                                       1);
    
    // First, we will generate rays on the GPU. We create a compute command encoder which will be used to add
    // commands to the command buffer.
    id <MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];
    
    // Bind buffers needed by the compute pipeline
    [computeEncoder setBuffer:_uniformBuffer   offset:_uniformBufferOffset atIndex:0];
    [computeEncoder setBuffer:_rayBuffer       offset:0                    atIndex:1];
    
    [computeEncoder setTexture:_randomTexture    atIndex:0];
    [computeEncoder setTexture:_renderTargets[0] atIndex:1];
    
    // Bind the ray generation compute pipeline
    [computeEncoder setComputePipelineState:_rayPipeline];
    
    // Launch threads
    [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
    
    // End the encoder
    [computeEncoder endEncoding];
    
    // We will iterate over the next few kernels several times to allow light to bounce around the scene
    for (int bounce = 0; bounce < 3; bounce++) {
        _intersector.intersectionDataType = MPSIntersectionDataTypeDistancePrimitiveIndexCoordinates;

        // We can then pass the rays to the MPSRayIntersector to compute the intersections with our acceleration structure
        [_intersector encodeIntersectionToCommandBuffer:commandBuffer               // Command buffer to encode into
                                       intersectionType:MPSIntersectionTypeNearest  // Intersection test type
                                              rayBuffer:_rayBuffer                  // Ray buffer
                                        rayBufferOffset:0                           // Offset into ray buffer
                                     intersectionBuffer:_intersectionBuffer         // Intersection buffer (destination)
                               intersectionBufferOffset:0                           // Offset into intersection buffer
                                               rayCount:width * height              // Number of rays
                                  accelerationStructure:_accelerationStructure];    // Acceleration structure
        
        // We launch another pipeline to consume the intersection results and shade the scene
        computeEncoder = [commandBuffer computeCommandEncoder];
        
        [computeEncoder setBuffer:_uniformBuffer      offset:_uniformBufferOffset atIndex:0];
        [computeEncoder setBuffer:_rayBuffer          offset:0                    atIndex:1];
        [computeEncoder setBuffer:_shadowRayBuffer    offset:0                    atIndex:2];
        [computeEncoder setBuffer:_intersectionBuffer offset:0                    atIndex:3];
        [computeEncoder setBuffer:_vertexColorBuffer  offset:0                    atIndex:4];
        [computeEncoder setBuffer:_vertexNormalBuffer offset:0                    atIndex:5];
        [computeEncoder setBuffer:_triangleMaskBuffer offset:0                    atIndex:6];
        [computeEncoder setBytes:&bounce              length:sizeof(bounce)       atIndex:7];
        
        [computeEncoder setTexture:_randomTexture    atIndex:0];
        [computeEncoder setTexture:_renderTargets[0] atIndex:1];
        
        [computeEncoder setComputePipelineState:_shadePipeline];
        
        [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
        
        [computeEncoder endEncoding];
        
        // We intersect rays with the scene, except this time we are intersecting shadow rays. We only need
        // to know whether the shadows rays hit anything on the way to the light source, not which triangle
        // was intersected. Therefore, we can use the "any" intersection type to end the intersection search
        // as soon as any intersection is found. This is typically much faster than finding the nearest
        // intersection. We can also use MPSIntersectionDataTypeDistance, because we don't need the triangle
        // index and barycentric coordinates.
        _intersector.intersectionDataType = MPSIntersectionDataTypeDistance;
        
        [_intersector encodeIntersectionToCommandBuffer:commandBuffer
                                       intersectionType:MPSIntersectionTypeAny
                                              rayBuffer:_shadowRayBuffer
                                        rayBufferOffset:0
                                     intersectionBuffer:_intersectionBuffer
                               intersectionBufferOffset:0
                                               rayCount:width * height
                                  accelerationStructure:_accelerationStructure];
        
        // Finally, we launch a kernel which writes the color computed by the shading kernel into the
        // output image, but only if the corresponding shadow ray does not intersect anything on the way to
        // the light. If the shadow ray intersects a triangle before reaching the light source, the original
        // intersection point was in shadow.
        computeEncoder = [commandBuffer computeCommandEncoder];
        
        [computeEncoder setBuffer:_uniformBuffer      offset:_uniformBufferOffset atIndex:0];
        [computeEncoder setBuffer:_shadowRayBuffer    offset:0                    atIndex:1];
        [computeEncoder setBuffer:_intersectionBuffer offset:0                    atIndex:2];
        
        [computeEncoder setTexture:_renderTargets[0] atIndex:0];
        [computeEncoder setTexture:_renderTargets[1] atIndex:1];
        
        [computeEncoder setComputePipelineState:_shadowPipeline];
        
        [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
        
        [computeEncoder endEncoding];
        
        std::swap(_renderTargets[0], _renderTargets[1]);
    }

    // The final kernel averages the current frame's image with all previous frames to reduce noise due
    // random sampling of the scene.
    computeEncoder = [commandBuffer computeCommandEncoder];
        
    [computeEncoder setBuffer:_uniformBuffer      offset:_uniformBufferOffset atIndex:0];
    
    [computeEncoder setTexture:_renderTargets[0]       atIndex:0];
    [computeEncoder setTexture:_accumulationTargets[0] atIndex:1];
    [computeEncoder setTexture:_accumulationTargets[1] atIndex:2];
    
    [computeEncoder setComputePipelineState:_accumulatePipeline];
    
    [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
    
    [computeEncoder endEncoding];
    
    std::swap(_accumulationTargets[0], _accumulationTargets[1]);

    // Copy the resulting image into our view using the graphics pipeline since we can't write directly to
    // it with a compute kernel. We need to delay getting the current render pass descriptor as long as
    // possible to avoid stalling until the GPU/compositor release a drawable. The render pass descriptor
    // may be nil if the window has moved off screen.
    //MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    
    MTLClearColor color = MTLClearColorMake(0, 0, 0, 1);
    
    MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPassDescriptor.colorAttachments[0].clearColor = color;
    renderPassDescriptor.colorAttachments[0].loadAction  = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptor.colorAttachments[0].texture = surface.texture;

    if (renderPassDescriptor != nil) {
        // Create a render encoder
        id <MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];

        [renderEncoder setRenderPipelineState:_copyPipeline];
        
        [renderEncoder setFragmentTexture:_accumulationTargets[0] atIndex:0];

        // Draw a quad which fills the screen
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];

        [renderEncoder endEncoding];

        // Present the drawable to the screen
        [commandBuffer presentDrawable:surface];
    }

    // Finally, commit the command buffer so that the GPU can start executing
    [commandBuffer commit];
}

@end

using namespace toyraygun;

bool MetalRenderer::init()
{
    Renderer::init();
    
    Engine* engine = Engine::instance();
    
    CAMetalLayer* swapchain = (__bridge CAMetalLayer *)SDL_RenderGetMetalLayer(engine->getRenderer());
    const id<MTLDevice> gpu = swapchain.device;
    
    _MetalRenderer* renderer = [[_MetalRenderer alloc] initWithDevice: gpu
                                parentRenderer:this];
    [renderer resize: CGSizeMake(engine->getWidth(), engine->getHeight())];
    
    _swapchain = swapchain;
    _renderer = renderer;
    
    return true;
}

void MetalRenderer::destroy()
{

}

void MetalRenderer::loadScene(Scene *scene)
{
    _MetalRenderer* renderer = (_MetalRenderer*)_renderer;
    [renderer loadScene:scene];
}

void MetalRenderer::renderFrame()
{
    CAMetalLayer* swapchain = (CAMetalLayer*)_swapchain;
    _MetalRenderer* renderer = (_MetalRenderer*)_renderer;
    
    @autoreleasepool
    {
        id<CAMetalDrawable> surface = [swapchain nextDrawable];
        [renderer render: surface];
    }
}

