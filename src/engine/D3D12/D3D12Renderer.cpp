/*
 * Toy Raygun
 * MIT License: https://github.com/andr3wmac/ToyRaygun/LICENSE

 * This renderer is based on D3D12 Raytracing Sample from:
     https://github.com/microsoft/DirectX-Graphics-Samples/
 * By Microsoft
*/

#include "D3D12Renderer.h"
#include "D3D12Utilities.h"
#include "engine/Engine.h"
#include "engine/Texture.h"

#include <bx/math.h>

using namespace DirectX;

const wchar_t* D3D12Renderer::kPrimaryHitGroupName = L"PrimaryHitGroup";
const wchar_t* D3D12Renderer::kShadowHitGroupName = L"ShadowHitGroup";

D3D12Renderer::D3D12Renderer()
{

}

bool D3D12Renderer::init()
{
    Renderer::init();

    ReleaseWindowSizeDependentResources();
    ReleaseDeviceDependentResources();

    m_device = std::make_unique<D3D12Device>(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_UNKNOWN,
        kDefaultSwapChainBuffers,
        D3D_FEATURE_LEVEL_11_0,
        D3D12Device::c_RequireTearingSupport,
        UINT_MAX
        );

    m_device->SetWindow(GetActiveWindow(), m_width, m_height);
    m_device->InitializeDXGIAdapter();

    if (!m_device->IsRaytracingSupported())
    {
        OutputDebugString("ERROR: DirectX Raytracing is not supported by your OS, GPU and/or driver.");
        return false;
    }

    m_device->CreateDeviceResources();
    m_device->CreateWindowSizeDependentResources();

    return true;
}

// Update camera matrices passed into the shader.
void D3D12Renderer::updateUniforms()
{
    auto bufferIndex = m_device->GetCurrentBackBufferIndex();

    m_sceneCB[bufferIndex].frameIndex = m_frameIndex;
    m_sceneCB[bufferIndex].width = m_width;
    m_sceneCB[bufferIndex].height = m_height;

    m_sceneCB[bufferIndex].camera.position.set(m_eye);

    float invViewProjMtx[16];
    bx::mtxInverse(invViewProjMtx, m_viewProjMtx);
    m_sceneCB[bufferIndex].camera.invViewProjMtx.set(invViewProjMtx);

    m_sceneCB[bufferIndex].light.position.set(bx::Vec3(0.0f, 1.98f, 0.0f));
    m_sceneCB[bufferIndex].light.forward.set(bx::Vec3(0.0f, -1.0f, 0.0f));
    m_sceneCB[bufferIndex].light.right.set(bx::Vec3(0.25f, 0.0f, 0.0f));
    m_sceneCB[bufferIndex].light.up.set(bx::Vec3(0.0f, 0.0f, 0.25f));
    m_sceneCB[bufferIndex].light.color.set(bx::Vec3(1.0f, 1.0f, 1.0f));
}

// Initialize scene rendering parameters.
void D3D12Renderer::loadScene(toyraygun::Scene* scene)
{
    auto bufferIndex = m_device->GetCurrentBackBufferIndex();

    // Setup uniforms.
    updateUniforms();

    // Apply the initial values to all frames' buffer instances.
    for (auto& sceneCB : m_sceneCB)
    {
        sceneCB = m_sceneCB[bufferIndex];
    }

    // Initialize raytracing pipeline.

    // Create raytracing interfaces: raytracing device and commandlist.
    CreateRaytracingInterfaces();

    // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
    createRaytracingPipeline();

    // Create a heap for descriptors.
    CreateDescriptorHeap();

    // Load random tex
    createRandomTexture();

    // Build geometry to be used in the sample.
    BuildGeometry(scene);

    // Build raytracing acceleration structures from the generated geometry.
    BuildAccelerationStructures();

    // Create constant buffers for the geometry and the scene.
    CreateConstantBuffers();

    // Build shader tables, which define shaders and their local root arguments.
    BuildShaderTables();

    // Create an output 2D texture to store the raytracing result to.
    CreateWindowSizeDependentResources();

    // Create Pipelines
    createAccumulatePipeline();
    createPostProcessingPipeline();
}

// Create constant buffers.
void D3D12Renderer::CreateConstantBuffers()
{
    auto device = m_device->GetD3DDevice();
    auto frameCount = m_device->GetBackBufferCount();
    
    // Create the constant buffer memory and map the CPU and GPU addresses
    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    // Allocate one constant buffer per frame, since it gets updated every frame.
    size_t cbSize = frameCount * sizeof(AlignedSceneConstantBuffer);
    const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_perFrameConstants)));

    // Map the constant buffer and cache its heap pointers.
    // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_perFrameConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData)));
}

void D3D12Renderer::SerializeAndCreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    auto device = m_device->GetD3DDevice();
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<char*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

// Create raytracing device and command list.
void D3D12Renderer::CreateRaytracingInterfaces()
{
    auto device = m_device->GetD3DDevice();
    auto commandList = m_device->GetCommandList();

    ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)), "Couldn't get DirectX Raytracing interface for the device.\n");
    ThrowIfFailed(commandList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList)), "Couldn't get DirectX Raytracing interface for the command list.\n");
}

// Local root signature and shader association
// This is a root signature that enables a shader to have unique arguments that come from shader tables.
void D3D12Renderer::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
    // Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

    // Local root signature to be used in a hit group.
    auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());
    // Define explicit shader association for the local root signature. 
    {
        auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(kPrimaryHitGroupName);
        rootSignatureAssociation->AddExport(kShadowHitGroupName);
    }
}

// Create 2D output texture for raytracing.
void D3D12Renderer::createTexture(D3D12Texture& texture, DXGI_FORMAT format)
{
    auto device = m_device->GetD3DDevice();

    // Create the output resource. The dimensions and format should match the swap-chain.
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&texture.resource)));
    NAME_D3D12_OBJECT(texture.resource);

    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
    texture.uavHeapIndex = AllocateDescriptor(&uavDescriptorHandle, texture.uavHeapIndex);
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(texture.resource.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
    texture.uavGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), texture.uavHeapIndex, m_descriptorSize);

    // Describe and create a SRV for the texture.
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle;
    texture.srvHeapIndex = AllocateDescriptor(&srvDescriptorHandle, texture.srvHeapIndex);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(texture.resource.Get(), &srvDesc, srvDescriptorHandle);
    texture.srvGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), texture.srvHeapIndex, m_descriptorSize);
}

void D3D12Renderer::CreateDescriptorHeap()
{
    auto device = m_device->GetD3DDevice();

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    // Allocate a heap for 4 descriptors:
    // 0 - raytracing output texture SRV
    // 1 - index buffer
    // 2 - vertex buffer 
    // 3 - material id buffer
    // 4 - random texture
    // 5 - accumulate texture
    // 6 - post processing texture
    descriptorHeapDesc.NumDescriptors = 12; 
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descriptorHeapDesc.NodeMask = 0;
    device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    NAME_D3D12_OBJECT(m_descriptorHeap);

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

// Build geometry used in the sample.
void D3D12Renderer::BuildGeometry(toyraygun::Scene* scene)
{
    auto device = m_device->GetD3DDevice();

    std::vector<Index> indices;
    for (int i = 0; i < scene->m_indexBuffer.size(); ++i)
    {
        indices.push_back(scene->m_indexBuffer[i]);
    }

    std::vector<Vertex> vertices;
    for (int i = 0; i < scene->m_vertexBuffer.size(); ++i)
    {
        vertices.push_back({ 
            XMFLOAT3(scene->m_vertexBuffer[i].x, scene->m_vertexBuffer[i].y, scene->m_vertexBuffer[i].z),
            XMFLOAT3(scene->m_normalBuffer[i].x, scene->m_normalBuffer[i].y, scene->m_normalBuffer[i].z),
            XMFLOAT3(scene->m_colorBuffer[i].x, scene->m_colorBuffer[i].y, scene->m_colorBuffer[i].z),
        });
    }

    std::vector<uint32_t> materialIDs;
    for (int i = 0; i < scene->m_materialIDBuffer.size(); ++i)
    {
        materialIDs.push_back(scene->m_materialIDBuffer[i]);
    }

    AllocateUploadBuffer(device, &indices[0], sizeof(Index) * indices.size(), &m_indexBuffer.resource);
    AllocateUploadBuffer(device, &vertices[0], sizeof(Vertex) * vertices.size(), &m_vertexBuffer.resource);
    AllocateUploadBuffer(device, &materialIDs[0], sizeof(uint32_t) * materialIDs.size(), &m_materialIDBuffer.resource);

    CreateBufferSRV(&m_indexBuffer, indices.size() / sizeof(Index), 0);
    CreateBufferSRV(&m_vertexBuffer, vertices.size(), sizeof(Vertex));
    CreateBufferSRV(&m_materialIDBuffer, materialIDs.size(), sizeof(uint32_t));
}

// Build acceleration structures needed for raytracing.
void D3D12Renderer::BuildAccelerationStructures()
{
    auto device = m_device->GetD3DDevice();
    auto commandList = m_device->GetCommandList();
    auto commandQueue = m_device->GetCommandQueue();
    auto commandAllocator = m_device->GetCommandAllocator();

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAllocator, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer.resource->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer.resource->GetDesc().Width) / sizeof(Index);
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer.resource->GetDesc().Width) / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer.resource->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = buildFlags;
    bottomLevelInputs.NumDescs = 1;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    ComPtr<ID3D12Resource> scratchResource;
    AllocateUAVBuffer(device, std::max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn’t need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    {
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        
        AllocateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
        AllocateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
    }
    
    // Create an instance desc for the bottom-level acceleration structure.
    ComPtr<ID3D12Resource> instanceDescs;   
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

    // Bottom Level Acceleration Structure desc
    {
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    }

    // Top Level Acceleration Structure desc
    {
        topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
    }

    auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
    {
        raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get()));
        raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    };

    // Build acceleration structure.
    BuildAccelerationStructure(m_dxrCommandList.Get());
    
    // Kick off acceleration structure construction.
    m_device->ExecuteCommandList();

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    m_device->WaitForGpu();
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void D3D12Renderer::BuildShaderTables()
{
    auto device = m_device->GetD3DDevice();

    void* rayGenShaderIdentifier;
    void* missShaderIdentifier;
    void* hitGroupShaderIdentifier;
    void* shadowHitGroupShaderIdentifier;
    void* shadowMissShaderIdentifier;

    Shader* rtShader = getShader("Raytracing");

    auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
    {
        rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rtShader->getFunctionW(toyraygun::ShaderFunctionType::RayGen).c_str());
        missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rtShader->getFunctionW(toyraygun::ShaderFunctionType::Miss).c_str());
        hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(kPrimaryHitGroupName);
        shadowHitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(kShadowHitGroupName);
        shadowMissShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rtShader->getFunctionW(toyraygun::ShaderFunctionType::ShadowMiss).c_str());
    };

    // Get shader identifiers.
    UINT shaderIdentifierSize;
    {
        ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        ThrowIfFailed(m_dxrStateObject.As(&stateObjectProperties));
        GetShaderIdentifiers(stateObjectProperties.Get());
        shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    }

    // Ray gen shader table
    {
        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize;
        ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
        rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
        m_rayGenShaderTable = rayGenShaderTable.GetResource();
    }

    // Miss shader table
    {
        UINT numShaderRecords = 2;
        UINT shaderRecordSize = shaderIdentifierSize;
        ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
        missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
        missShaderTable.push_back(ShaderRecord(shadowMissShaderIdentifier, shaderIdentifierSize));
        m_missShaderTable = missShaderTable.GetResource();
    }

    // Hit group shader table
    {
        UINT numShaderRecords = 2;
        UINT shaderRecordSize = shaderIdentifierSize;
        ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
        hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
        hitGroupShaderTable.push_back(ShaderRecord(shadowHitGroupShaderIdentifier, shaderIdentifierSize));
        m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
    }
}

// Update the application state with the new resolution.
void D3D12Renderer::UpdateForSizeChange(UINT width, UINT height)
{
    //DXSample::UpdateForSizeChange(width, height);
}

void D3D12Renderer::createOutputTextures()
{
    createTexture(m_raytracingOutput, DXGI_FORMAT_R32G32B32A32_FLOAT);
    createTexture(m_accumulateOutput, DXGI_FORMAT_R32G32B32A32_FLOAT);
    createTexture(m_postProcessingOutput, m_device->GetBackBufferFormat());
}

// Create resources that are dependent on the size of the main window.
void D3D12Renderer::CreateWindowSizeDependentResources()
{
    createOutputTextures();
    updateUniforms();
}

// Release resources that are dependent on the size of the main window.
void D3D12Renderer::ReleaseWindowSizeDependentResources()
{
    m_raytracingOutput.resource.Reset();
    m_accumulateOutput.resource.Reset();
    m_postProcessingOutput.resource.Reset();
    //m_randomTexture.Reset();
}

// Release all resources that depend on the device.
void D3D12Renderer::ReleaseDeviceDependentResources()
{
    m_raytracingGlobalRootSignature.Reset();
    m_raytracingLocalRootSignature.Reset();
    m_accumulateRootSignature.Reset();
    m_accumlateStateObject.Reset();

    m_dxrDevice.Reset();
    m_dxrCommandList.Reset();
    m_dxrStateObject.Reset();

    m_descriptorHeap.Reset();
    m_descriptorsAllocated = 0;
    m_indexBuffer.resource.Reset();
    m_vertexBuffer.resource.Reset();
    m_materialIDBuffer.resource.Reset();
    m_perFrameConstants.Reset();
    m_rayGenShaderTable.Reset();
    m_missShaderTable.Reset();
    m_hitGroupShaderTable.Reset();

    m_bottomLevelAccelerationStructure.Reset();
    m_topLevelAccelerationStructure.Reset();

    m_raytracingOutput.Reset();
    m_accumulateOutput.Reset();
    m_postProcessingOutput.Reset();
}

void D3D12Renderer::RecreateD3D()
{
    // Give GPU a chance to finish its execution in progress.
    try
    {
        m_device->WaitForGpu();
    }
    catch (HrException&)
    {
        // Do nothing, currently attached adapter is unresponsive.
    }
    m_device->HandleDeviceLost();
}

// Render the scene.
void D3D12Renderer::renderFrame()
{
    if (!m_device->IsWindowVisible())
    {
        return;
    }

    // Base class does some house keeping.
    Renderer::renderFrame();

    m_device->Prepare();

    updateUniforms();

    performRaytracing();
    performAccumulate();
    performPostProcessing();

    m_device->Present(D3D12_RESOURCE_STATE_PRESENT);
}

void D3D12Renderer::destroy()
{
    // Let GPU finish before releasing D3D resources.
    m_device->WaitForGpu();

    ReleaseWindowSizeDependentResources();
    ReleaseDeviceDependentResources();
}

// Handle OnSizeChanged message event.
/*void D3D12Renderer::OnSizeChanged(UINT width, UINT height, bool minimized)
{
    if (!m_deviceResources->WindowSizeChanged(width, height, minimized))
    {
        return;
    }

    UpdateForSizeChange(width, height);

    ReleaseWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}*/

// Allocate a descriptor and return its index. 
// If the passed descriptorIndexToUse is valid, it will be used instead of allocating a new one.
UINT D3D12Renderer::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse)
{
    auto descriptorHeapCpuBase = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    if (descriptorIndexToUse >= m_descriptorHeap->GetDesc().NumDescriptors)
    {
        descriptorIndexToUse = m_descriptorsAllocated++;
    }
    *cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_descriptorSize);
    return descriptorIndexToUse;
}

// Create SRV for a buffer.
UINT D3D12Renderer::CreateBufferSRV(D3D12Buffer* buffer, UINT numElements, UINT elementSize)
{
    auto device = m_device->GetD3DDevice();

    // SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = numElements;
    if (elementSize == 0)
    {
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.StructureByteStride = 0;
    }
    else
    {
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = elementSize;
    }
    UINT descriptorIndex = AllocateDescriptor(&buffer->cpuDescriptorHandle);
    device->CreateShaderResourceView(buffer->resource.Get(), &srvDesc, buffer->cpuDescriptorHandle);
    buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, m_descriptorSize);
    return descriptorIndex;
}

// Create 2d texture with random values for noise
void D3D12Renderer::createRandomTexture()
{
    auto device = m_device->GetD3DDevice();
    auto commandList = m_device->GetCommandList();
    auto backbufferFormat = m_device->GetBackBufferFormat();
    auto commandAllocator = m_device->GetCommandAllocator();

    // Reset the command list for the random texture construction.
    commandList->Reset(commandAllocator, nullptr);

    toyraygun::Texture blueNoiseTex;
    blueNoiseTex.loadFile("textures/stbn_vec3_2Dx1D_128x128x64_0.png");

    // Create the output resource. The dimensions and format should match the swap-chain.
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, blueNoiseTex.getWidth(), blueNoiseTex.getHeight(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &uavDesc, 
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, 
        IID_PPV_ARGS(&m_randomTexture.resource)));
    NAME_D3D12_OBJECT(m_randomTexture.resource);

    // Generate Random Texture
    {
        const UINT subresourceCount = uavDesc.DepthOrArraySize * uavDesc.MipLevels;
        UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_randomTexture.resource.Get(), 0, subresourceCount);
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_randomTextureUpload)));

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the Texture2D.
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = blueNoiseTex.getBufferPointer();
        textureData.RowPitch = blueNoiseTex.getBufferStride();
        textureData.SlicePitch = blueNoiseTex.getBufferSize();

        UpdateSubresources(commandList, m_randomTexture.resource.Get(), m_randomTextureUpload.Get(), 0, 0, subresourceCount, &textureData);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_randomTexture.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
    m_randomTexture.uavHeapIndex = AllocateDescriptor(&uavDescriptorHandle, m_randomTexture.uavHeapIndex);
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_randomTexture.resource.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
    m_randomTexture.uavGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_randomTexture.uavHeapIndex, m_descriptorSize);

    // Describe and create a SRV for the texture.
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle;
    m_randomTexture.srvHeapIndex = AllocateDescriptor(&srvDescriptorHandle, m_randomTexture.srvHeapIndex);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = backbufferFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_randomTexture.resource.Get(), &srvDesc, srvDescriptorHandle);
    m_randomTexture.srvGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_randomTexture.srvHeapIndex, m_descriptorSize);

    // Kick off acceleration structure construction.
    m_device->ExecuteCommandList();

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    m_device->WaitForGpu();
}

// Create a raytracing pipeline state object (RTPSO).
// An RTPSO represents a full set of shaders reachable by a DispatchRays() call,
// with all configuration options resolved, such as local signatures and other state.
void D3D12Renderer::createRaytracingPipeline()
{
    auto device = m_device->GetD3DDevice();

    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[5]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // Output
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);  // Random Texture
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);  // Index
        ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);  // Vertex
        ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);  // MaterialIDs

        CD3DX12_ROOT_PARAMETER rootParameters[7];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0]); // Output
        rootParameters[1].InitAsShaderResourceView(0);          // Acceleration Structure
        rootParameters[2].InitAsConstantBufferView(0);          // Uniforms
        rootParameters[3].InitAsDescriptorTable(1, &ranges[1]); // Random Texture
        rootParameters[4].InitAsDescriptorTable(1, &ranges[2]); // Index
        rootParameters[5].InitAsDescriptorTable(1, &ranges[3]); // Vertex
        rootParameters[6].InitAsDescriptorTable(1, &ranges[4]); // MaterialIDs

        // Sampler for random texture.
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 2;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters, 1, &sampler);
        SerializeAndCreateRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
    }

    // Local Root Signature would go here but not currently in use.

    /* Example of working RTPSO:
    --------------------------------------------------------------------
    | D3D12 State Object 0x000000ADFDCFE7C0: Raytracing Pipeline
    | [0]: DXIL Library 0x0000023088A1E8E0, 11368 bytes
    |  [0]: raygen
    |  [1]: primaryHit
    |  [2]: primaryMiss
    |  [3]: shadowHit
    |  [4]: shadowMiss
    |--------------------------------------------------------------------
    | [1]: Hit Group (PrimaryHitGroup)
    |  [0]: Any Hit Import: [none]
    |  [1]: Closest Hit Import: primaryHit
    |  [2]: Intersection Import: [none]
    |--------------------------------------------------------------------
    | [2]: Hit Group (ShadowHitGroup)
    |  [0]: Any Hit Import: [none]
    |  [1]: Closest Hit Import: shadowHit
    |  [2]: Intersection Import: [none]
    |--------------------------------------------------------------------
    | [3]: Raytracing Shader Config
    |  [0]: Max Payload Size: 16 bytes
    |  [1]: Max Attribute Size: 8 bytes
    |--------------------------------------------------------------------
    | [4]: Local Root Signature 0x0000023089125580
    |--------------------------------------------------------------------
    | [5]: Subobject to Exports Association (Subobject [4])
    |  [0]: PrimaryHitGroup
    |  [1]: ShadowHitGroup
    |--------------------------------------------------------------------
    | [6]: Global Root Signature 0x00000230891260C0
    |--------------------------------------------------------------------
    | [7]: Raytracing Pipeline Config
    |  [0]: Max Recursion Depth: 3
    |--------------------------------------------------------------------
    */

    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    Shader* rtShader = getShader("Raytracing");

    // [0] : DXIL Library
    // Load precompiled shader that was set for the renderer.
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(rtShader->getBufferPointer(), rtShader->getBufferSize());
    lib->SetDXILLibrary(&libdxil);

    // Export each specified function to use in the pipeline.
    {
        std::vector<std::string> functionNames = rtShader->getFunctionNames();
        for (int i = 0; i < functionNames.size(); ++i)
        {
            std::wstring funcNameW = std::wstring(functionNames[i].begin(), functionNames[i].end());
            lib->DefineExport(funcNameW.c_str());
        }
    }

    // [1] : Hit Group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
    auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetClosestHitShaderImport(rtShader->getFunctionW(toyraygun::ShaderFunctionType::ClosestHit).c_str());
    hitGroup->SetHitGroupExport(kPrimaryHitGroupName);
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    // [2] : Shadow Hit Group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
    auto shadowHitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    shadowHitGroup->SetClosestHitShaderImport(rtShader->getFunctionW(toyraygun::ShaderFunctionType::ShadowHit).c_str());
    shadowHitGroup->SetHitGroupExport(kShadowHitGroupName);
    shadowHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    // [3]: Raytracing Shader Config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = sizeof(XMFLOAT4) + sizeof(UINT);    // float4 pixelColor
    UINT attributeSize = sizeof(XMFLOAT2);  // float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);

    // [4]: Local Root Signature
    // [5]: Subobject to Exports Association
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    CreateLocalRootSignatureSubobjects(&raytracingPipeline);

    // [6]: Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.Get());

    // [7]: Raytracing Pipeline Config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();

    // PERFORMANCE TIP: Set max recursion depth as low as needed 
    // as drivers may apply optimization strategies for low recursion depths.
    UINT maxRecursionDepth = 3; // ~ primary rays only. 
    pipelineConfig->Config(maxRecursionDepth);

#if _DEBUG
    PrintStateObjectDesc(raytracingPipeline);
#endif

    // Create the state object.
    ThrowIfFailed(m_dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), "Couldn't create DirectX Raytracing state object.\n");
}

void D3D12Renderer::performRaytracing()
{
    auto commandList = m_device->GetCommandList();
    auto bufferIndex = m_device->GetCurrentBackBufferIndex();

    // Global root signature
    commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());

    // Copy the updated scene constant buffer to GPU.
    memcpy(&m_mappedConstantData[bufferIndex].constants, &m_sceneCB[bufferIndex], sizeof(m_sceneCB[bufferIndex]));
    auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + bufferIndex * sizeof(m_mappedConstantData[0]);

    // Bind the heaps, acceleration structure and dispatch rays.
    commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());

    // Set index and successive vertex buffer decriptor tables
    commandList->SetComputeRootDescriptorTable(0, m_raytracingOutput.uavGPUHandle);
    commandList->SetComputeRootShaderResourceView(1, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(2, cbGpuAddress);
    commandList->SetComputeRootDescriptorTable(3, m_randomTexture.srvGPUHandle);
    commandList->SetComputeRootDescriptorTable(4, m_indexBuffer.gpuDescriptorHandle);
    commandList->SetComputeRootDescriptorTable(5, m_vertexBuffer.gpuDescriptorHandle);
    commandList->SetComputeRootDescriptorTable(6, m_materialIDBuffer.gpuDescriptorHandle);

    // Since each shader table has only one shader record, the stride is same as the size.
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

    dispatchDesc.HitGroupTable.StartAddress    = m_hitGroupShaderTable->GetGPUVirtualAddress();
    dispatchDesc.HitGroupTable.SizeInBytes     = m_hitGroupShaderTable->GetDesc().Width;
    dispatchDesc.HitGroupTable.StrideInBytes   = dispatchDesc.HitGroupTable.SizeInBytes / 2;

    dispatchDesc.MissShaderTable.StartAddress  = m_missShaderTable->GetGPUVirtualAddress();
    dispatchDesc.MissShaderTable.SizeInBytes   = m_missShaderTable->GetDesc().Width;
    dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes / 2;

    dispatchDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes  = m_rayGenShaderTable->GetDesc().Width;

    dispatchDesc.Width  = m_width;
    dispatchDesc.Height = m_height;
    dispatchDesc.Depth  = 1;

    m_dxrCommandList->SetPipelineState1(m_dxrStateObject.Get());
    m_dxrCommandList->DispatchRays(&dispatchDesc);
}

void D3D12Renderer::createAccumulatePipeline()
{
    auto device = m_device->GetD3DDevice();

    // Accumulate Root Signature
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[3]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);  // random texture
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);  // output texture
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);  // accumulate texture

        CD3DX12_ROOT_PARAMETER rootParameters[4];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1]);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2]);
        rootParameters[3].InitAsConstantBufferView(0);

        CD3DX12_ROOT_SIGNATURE_DESC accumRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        SerializeAndCreateRootSignature(accumRootSignatureDesc, &m_accumulateRootSignature);
    }

    Shader* accumulateShader = getShader("Accumulate");

    // Describe and create the compute pipeline state object (PSO).
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_accumulateRootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(accumulateShader->getBufferPointer(), accumulateShader->getBufferSize());

    ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_accumlateStateObject)));
    NAME_D3D12_OBJECT(m_accumlateStateObject);
}

// Accumulate output from raytracing.
void D3D12Renderer::performAccumulate()
{
    auto commandList = m_device->GetCommandList();
    auto renderTarget = m_device->GetRenderTarget();
    auto bufferIndex = m_device->GetCurrentBackBufferIndex();

    D3D12_RESOURCE_BARRIER preDispatchBarriers[2];
    preDispatchBarriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_raytracingOutput.resource.Get());
    preDispatchBarriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_accumulateOutput.resource.Get());
    commandList->ResourceBarrier(ARRAYSIZE(preDispatchBarriers), preDispatchBarriers);

    commandList->SetPipelineState(m_accumlateStateObject.Get());
    commandList->SetComputeRootSignature(m_accumulateRootSignature.Get());

    commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    commandList->SetComputeRootDescriptorTable(0, m_randomTexture.srvGPUHandle);
    commandList->SetComputeRootDescriptorTable(1, m_raytracingOutput.uavGPUHandle);
    commandList->SetComputeRootDescriptorTable(2, m_accumulateOutput.uavGPUHandle);

    // Copy the updated scene constant buffer to GPU.
    memcpy(&m_mappedConstantData[bufferIndex].constants, &m_sceneCB[bufferIndex], sizeof(m_sceneCB[bufferIndex]));
    auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + bufferIndex * sizeof(m_mappedConstantData[0]);
    commandList->SetComputeRootConstantBufferView(3, cbGpuAddress);

    commandList->Dispatch(m_width, m_height, 1);
}

void D3D12Renderer::createPostProcessingPipeline()
{
    auto device = m_device->GetD3DDevice();

    // Root Signature
    {
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        CD3DX12_DESCRIPTOR_RANGE ranges[1]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER rootParameters[2];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsConstantBufferView(0);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_ROOT_SIGNATURE_DESC accumRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);
        SerializeAndCreateRootSignature(accumRootSignatureDesc, &m_postProcessingRootSignature);
    }

    Shader* postProcessingShader = getShader("PostProcessing");

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_postProcessingRootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(postProcessingShader->getBufferPointer(ShaderFunctionType::Vertex), postProcessingShader->getBufferSize(ShaderFunctionType::Vertex));
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(postProcessingShader->getBufferPointer(ShaderFunctionType::Fragment), postProcessingShader->getBufferSize(ShaderFunctionType::Fragment));
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_postProcessingStateObject)));
    NAME_D3D12_OBJECT(m_postProcessingStateObject);

    struct QuadVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    // Instead of a full screen quad we use a triangle thats twice the width and height
    // of the screen, then double the UVs so they're 0-1 over the area of the screen.
    std::vector<QuadVertex> vertices;
    vertices.push_back({ XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }); // Top Left
    vertices.push_back({ XMFLOAT3( 3.0f,  1.0f, 0.0f), XMFLOAT2(2.0f, 0.0f) }); // Top Right
    vertices.push_back({ XMFLOAT3(-1.0f, -3.0f, 0.0f), XMFLOAT2(0.0f, 2.0f) }); // Bottom Left

    AllocateUploadBuffer(device, &vertices[0], sizeof(QuadVertex) * vertices.size(), &m_quadVertexBuffer.resource);
    CreateBufferSRV(&m_quadVertexBuffer, vertices.size(), sizeof(QuadVertex));

    m_quadVertexBufferView.BufferLocation = m_quadVertexBuffer.resource->GetGPUVirtualAddress();
    m_quadVertexBufferView.StrideInBytes = sizeof(QuadVertex);
    m_quadVertexBufferView.SizeInBytes = vertices.size() * sizeof(QuadVertex);
}

// Copy the raytracing output to the backbuffer.
void D3D12Renderer::performPostProcessing()
{
    auto commandList = m_device->GetCommandList();
    auto renderTarget = m_device->GetRenderTarget();
    auto bufferIndex = m_device->GetCurrentBackBufferIndex();

    D3D12_RESOURCE_BARRIER preDrawBarriers[1];
    preDrawBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_accumulateOutput.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(ARRAYSIZE(preDrawBarriers), preDrawBarriers);

    commandList->SetPipelineState(m_postProcessingStateObject.Get());
    commandList->SetGraphicsRootSignature(m_postProcessingRootSignature.Get());

    commandList->RSSetViewports(1, &m_device->GetScreenViewport());
    commandList->RSSetScissorRects(1, &m_device->GetScissorRect());

    commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootDescriptorTable(0, m_accumulateOutput.srvGPUHandle);
        
    // Copy the updated scene constant buffer to GPU.
    memcpy(&m_mappedConstantData[bufferIndex].constants, &m_sceneCB[bufferIndex], sizeof(m_sceneCB[bufferIndex]));
    auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + bufferIndex * sizeof(m_mappedConstantData[0]);
    commandList->SetGraphicsRootConstantBufferView(1, cbGpuAddress);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_device->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Clear then draw full screen tri.
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_quadVertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0);

    // Switch accumulate back to UAV and indicate that the back buffer will now be used to present.
    D3D12_RESOURCE_BARRIER postDrawBarriers[2];
    postDrawBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_accumulateOutput.resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    postDrawBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(ARRAYSIZE(postDrawBarriers), postDrawBarriers);
}