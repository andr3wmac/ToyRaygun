/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE

 * This renderer is based on D3D12 Raytracing Sample from:
     https://github.com/microsoft/DirectX-Graphics-Samples/
 * By Microsoft 
*/

#pragma once

#include "engine/Engine.h"
#include "engine/Renderer.h"
#include "engine/D3D12/D3D12Shader.h"
#include "engine/D3D12/D3D12Device.h"
#include "engine/Uniforms.h"

#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <memory>

#include <atlbase.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

namespace GlobalRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        SceneConstantSlot,
        IndexBuffersSlot,
        VertexBuffersSlot,
        MaterialIDBufferSlot,
        RandomTextureSlot,
        Count 
    };
}

namespace LocalRootSignatureParams {
    enum Value {
        CubeConstantSlot = 0,
        Count 
    };
}

class D3D12Renderer : public toyraygun::Renderer
{
public:
    D3D12Renderer();

    // Setup
    virtual bool init();
    virtual void destroy();
    virtual void loadScene(toyraygun::Scene* scene);

    // Rendering
    virtual void renderFrame();

private:

    // Index Buffer Type
    typedef UINT16 Index;

    // Vertex Buffer Type
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT3 color;
    };

    static const UINT kDefaultSwapChainBuffers = 3;
    std::unique_ptr<D3D12Device> m_device;

    // We'll allocate space for several of these and they will need to be padded for alignment.
    static_assert(sizeof(toyraygun::Uniforms) < D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, "Checking the size here.");

    union AlignedSceneConstantBuffer
    {
        toyraygun::Uniforms constants;
        uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
    };
    AlignedSceneConstantBuffer* m_mappedConstantData;
    ComPtr<ID3D12Resource>      m_perFrameConstants;

    // DirectX Raytracing (DXR) attributes
    ComPtr<ID3D12Device5>              m_dxrDevice;
    ComPtr<ID3D12GraphicsCommandList5> m_dxrCommandList;
    ComPtr<ID3D12StateObject>          m_dxrStateObject;

    // Root signatures
    ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
    ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;

    // Descriptors
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    UINT m_descriptorsAllocated;
    UINT m_descriptorSize;
    
    // Scene
    toyraygun::Uniforms m_sceneCB[kDefaultSwapChainBuffers];
    D3D12Buffer m_indexBuffer;
    D3D12Buffer m_vertexBuffer;
    D3D12Buffer m_materialIDBuffer;

    // Acceleration structure
    ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
    ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

    // Raytracing input
    ComPtr<ID3D12Resource> m_randomTexture;
    ComPtr<ID3D12Resource> m_randomTextureUpload;
    D3D12_GPU_DESCRIPTOR_HANDLE m_randomTextureUAVGpuDescriptor;
    UINT m_randomTextureUAVDescriptorHeapIndex;

    // Raytracing output
    D3D12Texture m_raytracingOutput;

    // Raytracing shader tables
    static const wchar_t* kPrimaryHitGroupName;
    static const wchar_t* kShadowHitGroupName;
    ComPtr<ID3D12Resource> m_missShaderTable;
    ComPtr<ID3D12Resource> m_hitGroupShaderTable;
    ComPtr<ID3D12Resource> m_rayGenShaderTable;

    void createRaytracingPipeline();
    void performRaytracing();

    // Accumulate
    ComPtr<ID3D12RootSignature> m_accumulateRootSignature;
    ComPtr<ID3D12PipelineState> m_accumlateStateObject;
    D3D12Texture m_accumulateOutput;

    void createAccumulatePipeline();
    void performAccumulate();

    // Post Processing
    ComPtr<ID3D12RootSignature> m_postProcessingRootSignature;
    ComPtr<ID3D12PipelineState> m_postProcessingStateObject;
    D3D12Texture m_postProcessingOutput;
    D3D12Buffer m_quadVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_quadVertexBufferView;

    void createPostProcessingPipeline();
    void performPostProcessing();

    void createRandomTexture();
    void updateUniforms();
    void createOutputTextures();
    void createTexture(D3D12Texture& texture, DXGI_FORMAT format);

    // Other
    void RecreateD3D();
    void CreateConstantBuffers();
    void CreateWindowSizeDependentResources();
    void ReleaseDeviceDependentResources();
    void ReleaseWindowSizeDependentResources();
    void CreateRaytracingInterfaces();
    void SerializeAndCreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
    void CreateRootSignatures();
    void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
    void CreateDescriptorHeap();
    void BuildGeometry(toyraygun::Scene* scene);
    void BuildAccelerationStructures();
    void BuildShaderTables();
    void UpdateForSizeChange(UINT clientWidth, UINT clientHeight);
    UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse = UINT_MAX);
    UINT CreateBufferSRV(D3D12Buffer* buffer, UINT numElements, UINT elementSize);
};
