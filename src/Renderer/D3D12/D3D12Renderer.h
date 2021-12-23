#ifndef _RENDERERS_D3D12_
#define _RENDERERS_D3D12_

#include "../Renderer.h"

// include the basic windows header files and the Direct3D header files
#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

//helper class/functions for D3D12 taken from documentation pages
#include "helpers.h"

// global declarations
const UINT D3D12_BACKBUFFER_COUNT = 4; //define number of backbuffers to use

// Helper Functions
HRESULT CreateTexture2D(_In_ ID3D12Device* d3dDevice, _In_ ID3D12GraphicsCommandList* cmdList, _In_ CUploadBufferWrapper* uploadBuffer,
    _In_ const wchar_t* fileName, _Outptr_opt_ ID3D12Resource** resourceOut);

class D3D12Renderer : public Renderer
{
protected:
    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;							// d3d12 device
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandListAllocator;	// d3d12 command list allocator
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;				// d3d12 command queue
    Microsoft::WRL::ComPtr<IDXGIDevice2> mDXGIDevice;						// DXGI device
    Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;						// the pointer to the swap chain interface
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;			// d3d12 command list
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;								// fence used by GPU to signal when command queue execution has finished
    Microsoft::WRL::ComPtr<ID3D12Resource> mRenderTarget[D3D12_BACKBUFFER_COUNT];		// backbuffer resource, like d3d11's ID3D11Texture2D, array of 2 for flip_sequential support
    CDescriptorHeapWrapper mRTVDescriptorHeap;								// descriptor heap wrapper class instance, for managing RTV descriptor heap

    D3D12_VIEWPORT mViewPort;	// viewport, same as d3d11
    RECT mRectScissor;
    HANDLE mHandle;				// fired by the fence when the GPU signals it, CPU waits on this event handle

    //Constant buffer resources, mapped pointers, and descriptor heap for view/proj CBVs
    CUploadBufferWrapper mWorldMatrix;
    CUploadBufferWrapper mViewMatrix;
    CUploadBufferWrapper mProjMatrix;
    CDescriptorHeapWrapper mCBDescriptorHeap;

    //texture support
    Microsoft::WRL::ComPtr<ID3D12Resource> mTexture2D; //default heap resource, GPU will copy texture resource to these from upload buffer
    CDescriptorHeapWrapper mSamplerHeap;

private:
    void WaitForCommandQueueFence();
    HRESULT ResizeSwapChain();
    void SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, UINT StateBefore, UINT StateAfter);

public:
    void Init();
    void Destroy();

    void RenderFrame();
};

#endif