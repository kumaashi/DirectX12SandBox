#pragma once

#include <D3Dcompiler.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <pix.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>
#include <map>

#pragma  comment(lib, "dxgi.lib")
#pragma  comment(lib, "d3d12.lib")
#pragma  comment(lib, "D3DCompiler.lib")

#define RELEASE(x) {if(x) {x->Release(); x = NULL; } }
#define RELEASE_MAP(cont) {for(auto & a : cont) RELEASE( (a.second) ); cont.clear(); }

struct Device {
	typedef std::vector<unsigned char> D3DShaderVector;

	struct Vector {
		float data[4];
	};

	struct Matrix {
		float data[16];
	};

	struct VertexFormat {
		float position[3];
		float normal[3];
		float texcoord[2];
		float color[4];
	};

	struct InstancingData {
		Matrix world;
		Vector   color;
	};

	struct MatrixData {
		Matrix world  ;
		Matrix view   ;
		Matrix proj   ;
		Matrix padding;
		Vector time;
	};

	enum {
		Width                       = 1280,
		Height                      = 720,
		MAX_BACKBUFFER_0            = 0,
		MAX_BACKBUFFER_1            = MAX_BACKBUFFER_0 + 1,
		MAX_BACKBUFFER              = 2,
		MAX_RENDER_TARGET           = 64 + MAX_BACKBUFFER,
		MAX_MULTI_RENDER_TARGET_NUM = 4, //max -> D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT
		MAX_CONSTANT_BUFFER         = 32,
		MAX_TEXTURE                 = 256,
		PAGE_SIZE                   = 1,
		MAX_MIPLEVELS               = 1,
		MAX_RTV_HEAP                = PAGE_SIZE * (MAX_RENDER_TARGET),
		MAX_DSV_HEAP                = PAGE_SIZE * (MAX_RENDER_TARGET),
		MAX_SRV_HEAP                = PAGE_SIZE * (MAX_TEXTURE + MAX_RENDER_TARGET),
		MAX_CBV_HEAP                = PAGE_SIZE * (MAX_CONSTANT_BUFFER),
		MAX_UAV_HEAP                = MAX_SRV_HEAP,
		MAX_VERTEX_BUFFER           = 256,
		MAX_POLY_COUNT              = 32768,
		MAX_VERTEX_COUNT            = MAX_POLY_COUNT * 3,
		MAX_CONSTANT_BUFFER_SIZE    = 32768,
		MAX_HLSL = 8,
	};

	ID3D12Device              *device      = nullptr;
	ID3D12CommandQueue        *queue       = nullptr;
	ID3D12CommandAllocator    *allocator   = nullptr;
	ID3D12GraphicsCommandList *commandlist = nullptr;
	ID3D12Fence               *fence       = nullptr;
	IDXGISwapChain3           *swapchain   = nullptr;
	IDXGIFactory4             *factory     = nullptr;
	HANDLE                     fenceEvent  = nullptr;
	UINT64                     fenceValue  = 0;
	
	ID3D12DescriptorHeap *rtv_descripter_heap = nullptr;
	ID3D12DescriptorHeap *dsv_descripter_heap = nullptr;
	ID3D12DescriptorHeap *cbv_descripter_heap = nullptr;
	ID3D12DescriptorHeap *srv_descripter_heap = nullptr;

	std::map<int, D3D12_VERTEX_BUFFER_VIEW> vertex_views;
	std::map<int, D3D12_CONSTANT_BUFFER_VIEW_DESC> constant_views;
	std::map<int, D3D12_SHADER_RESOURCE_VIEW_DESC> shader_views;

	std::map<int, ID3D12Resource *> rtv_resources;
	std::map<int, ID3D12Resource *> dsv_resources;
	std::map<int, ID3D12Resource *> cbv_resources;
	std::map<int, ID3D12Resource *> srv_resources;
	std::map<int, ID3D12Resource *> vertex_resources;
	std::map<int, ID3D12RootSignature *> root_signatures;
	std::map<int, ID3D12PipelineState *> pipeline_states;
	std::map<int, D3D12_CPU_DESCRIPTOR_HANDLE> rtv_descripter_handle;
	std::map<int, D3D12_CPU_DESCRIPTOR_HANDLE> dsv_descripter_handle;
	std::map<int, D3D12_CPU_DESCRIPTOR_HANDLE> cbv_descripter_handle;
	std::map<int, D3D12_CPU_DESCRIPTOR_HANDLE> srv_descripter_handle;
	std::map<int, D3D12_GPU_DESCRIPTOR_HANDLE> rtv_gpu_descripter_handle;
	std::map<int, D3D12_GPU_DESCRIPTOR_HANDLE> dsv_gpu_descripter_handle;
	std::map<int, D3D12_GPU_DESCRIPTOR_HANDLE> cbv_gpu_descripter_handle;
	std::map<int, D3D12_GPU_DESCRIPTOR_HANDLE> srv_gpu_descripter_handle;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(int index, int offset = 0)    { return rtv_descripter_handle[offset + index * PAGE_SIZE];     }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(int index, int offset = 0)    { return dsv_descripter_handle[offset + index * PAGE_SIZE];     }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCBVHandle(int index, int offset = 0)    { return cbv_descripter_handle[offset + index * PAGE_SIZE];     }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle(int index, int offset = 0)    { return srv_descripter_handle[offset + index * PAGE_SIZE];     }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPURTVHandle(int index, int offset = 0) { return rtv_gpu_descripter_handle[offset + index * PAGE_SIZE]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDSVHandle(int index, int offset = 0) { return dsv_gpu_descripter_handle[offset + index * PAGE_SIZE]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCBVHandle(int index, int offset = 0) { return cbv_gpu_descripter_handle[offset + index * PAGE_SIZE]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSRVHandle(int index, int offset = 0) { return srv_gpu_descripter_handle[offset + index * PAGE_SIZE]; }

	D3D12_RESOURCE_BARRIER BeginBarrier[MAX_MULTI_RENDER_TARGET_NUM] = {};
	D3D12_RESOURCE_BARRIER EndBarrier[MAX_MULTI_RENDER_TARGET_NUM] = {};

	ID3D12Resource *CreateCommittedResource(
	    UINT width,
	    UINT height = 1,
	    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
	    D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE,
	    D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_GENERIC_READ)
	{
		ID3D12Resource *ret = nullptr;
		D3D12_HEAP_PROPERTIES heap_prop  = {};
		heap_prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
		heap_prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
		heap_prop.CreationNodeMask       = 1;
		heap_prop.VisibleNodeMask        = 1;

		D3D12_RESOURCE_DESC resource_desc = {};
		resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resource_desc.Alignment          = 0;
		resource_desc.Width              = width;
		resource_desc.Height             = height;
		resource_desc.DepthOrArraySize   = 1;
		resource_desc.MipLevels          = MAX_MIPLEVELS;
		resource_desc.Format             = format;
		resource_desc.SampleDesc.Count   = 1;
		resource_desc.SampleDesc.Quality = 0;
		resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resource_desc.Flags              = flag;
		if (height <= 1) {
			resource_desc.MipLevels          = 1;
			resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			heap_prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		}

		device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, resource_state, 0, IID_PPV_ARGS(&ret));
		return ret;
	}

	UINT64 WaitForSignalCommandComplete(ID3D12CommandQueue *queue, ID3D12Fence *fence, HANDLE fenceEvent, UINT64 fenceValue) {
		const UINT64 fenceSignalValue = fenceValue;
		queue->Signal(fence, fenceSignalValue);
		fenceValue++;
		if (fence->GetCompletedValue() < fenceSignalValue) {
			fence->SetEventOnCompletion(fenceSignalValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		return fenceValue;
	}

	D3D12_RESOURCE_BARRIER ResourceBarrier(
	    ID3D12Resource *resource,
	    D3D12_RESOURCE_STATES before,
	    D3D12_RESOURCE_STATES after,
	    D3D12_RESOURCE_BARRIER_TYPE type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
	{
		D3D12_RESOURCE_BARRIER ret = {};
		ret.Type = type;
		ret.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ret.Transition.pResource = resource;
		ret.Transition.StateBefore = before;
		ret.Transition.StateAfter = after;
		ret.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		return ret;
	}

	void UploadResourceSubData(ID3D12Resource *resource, void *data, UINT size) {
		UINT8 *pData = NULL;
		resource->Map(0, NULL, reinterpret_cast<void**>(&pData));
		if (pData) {
			memcpy(pData, data, size);
			resource->Unmap(0, NULL);
		} else {
			printf("Failed Map\n");
		}
	}

	void CompileShaderFromFile(D3DShaderVector &vdest, const char *filename, const char *entryname, const char *profile, UINT flags, ID3DBlob **blobsig = 0) {
		ID3DBlob *blob = NULL;
		ID3DBlob *blobError = NULL;
		if (!filename) {
			printf("%s : Invalid Filename\n", __FUNCTION__);
			return;
		}

		std::string fstr = filename;
		std::vector<WCHAR> fname;
		for (int i = 0 ; i < fstr.length(); i++) {
			fname.push_back(filename[i]);
		}
		fname.push_back(0);

		D3DCompileFromFile(&fname[0], NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryname, profile, flags, 0, &blob, &blobError);
		if (blob) {
			vdest.resize(blob->GetBufferSize());
			memcpy(&vdest[0], blob->GetBufferPointer(), vdest.size());
		} else {
			if (blobError) {
				printf("INFO : %s\n", blobError->GetBufferPointer());
			} else {
				printf("File Not found : %s\n", filename);
			}
		}

		//https://glhub.blogspot.jp/2016/08/dx12-hlslroot-signature.html
		if (blob && blobsig) {
			D3DGetBlobPart(blob->GetBufferPointer(), blob->GetBufferSize(), D3D_BLOB_ROOT_SIGNATURE, 0, blobsig);
		}
		RELEASE(blob);
	}

	~Device() {
		RELEASE_MAP(pipeline_states);
		RELEASE_MAP(root_signatures);
		RELEASE_MAP(vertex_resources);
		RELEASE_MAP(srv_resources);
		RELEASE_MAP(cbv_resources);
		RELEASE_MAP(dsv_resources);
		RELEASE_MAP(rtv_resources);

		RELEASE(dsv_descripter_heap);
		RELEASE(rtv_descripter_heap);
		RELEASE(cbv_descripter_heap);
		RELEASE(srv_descripter_heap);
		RELEASE(swapchain);
		RELEASE(factory);
		RELEASE(fence);
		RELEASE(allocator);
		RELEASE(commandlist);
		RELEASE(queue);
		RELEASE(device);
	}

	void ReloadPipeline() {
		RELEASE_MAP(root_signatures);
		RELEASE_MAP(pipeline_states);

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UV",            0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT,           1,  0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		};

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
		inputLayoutDesc.pInputElementDescs      = inputElementDescs;
		inputLayoutDesc.NumElements             = _countof(inputElementDescs);
		
		D3D12_RASTERIZER_DESC rasterizerState = {};
		rasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
		rasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
		rasterizerState.FrontCounterClockwise = FALSE;
		rasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizerState.DepthClipEnable       = TRUE;
		rasterizerState.MultisampleEnable     = FALSE;
		rasterizerState.AntialiasedLineEnable = FALSE;
		rasterizerState.ForcedSampleCount     = 0;
		rasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		
		D3D12_DEPTH_STENCIL_DESC depthStencilState = {};
		depthStencilState.DepthEnable                  = TRUE;
		depthStencilState.DepthFunc                    = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilState.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilState.StencilEnable                = FALSE;
		depthStencilState.StencilReadMask              = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilState.StencilWriteMask             = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		depthStencilState.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
		depthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		depthStencilState.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
		depthStencilState.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
		depthStencilState.BackFace.StencilFailOp       = D3D12_STENCIL_OP_KEEP;
		depthStencilState.BackFace.StencilDepthFailOp  = D3D12_STENCIL_OP_KEEP;
		depthStencilState.BackFace.StencilPassOp       = D3D12_STENCIL_OP_KEEP;
		depthStencilState.BackFace.StencilFunc         = D3D12_COMPARISON_FUNC_ALWAYS;

		D3D12_RENDER_TARGET_BLEND_DESC defaultBlendDesc = {};
		defaultBlendDesc.BlendEnable           = FALSE;
		defaultBlendDesc.LogicOpEnable         = FALSE;
		defaultBlendDesc.SrcBlend              = D3D12_BLEND_SRC_ALPHA ;
		//defaultBlendDesc.DestBlend             = D3D12_BLEND_ONE     ; //D3D12_BLEND_INV_DEST_ALPHA
		defaultBlendDesc.DestBlend             = D3D12_BLEND_INV_DEST_ALPHA;
		defaultBlendDesc.BlendOp               = D3D12_BLEND_OP_ADD;
		defaultBlendDesc.SrcBlendAlpha         = D3D12_BLEND_ONE;
		defaultBlendDesc.DestBlendAlpha        = D3D12_BLEND_ZERO;
		defaultBlendDesc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
		defaultBlendDesc.LogicOp               = D3D12_LOGIC_OP_NOOP;
		defaultBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		for(int i = 0 ; i < MAX_HLSL; i++) {
			D3DShaderVector vscode;
			D3DShaderVector gscode;
			D3DShaderVector pscode;
			ID3DBlob *blobsig = NULL;
			char shader_filename[256];
			sprintf(shader_filename, "effect%d.hlsl", i);
			CompileShaderFromFile(vscode, shader_filename, "VSMain", "vs_5_0", 0, &blobsig);
			CompileShaderFromFile(gscode, shader_filename, "GSMain", "gs_5_0", 0);
			CompileShaderFromFile(pscode, shader_filename, "PSMain", "ps_5_0", 0);
			if(!vscode.empty() && !vscode.empty()) {
				ID3D12RootSignature *signature = nullptr;
				device->CreateRootSignature(0, blobsig->GetBufferPointer(), blobsig->GetBufferSize(), IID_PPV_ARGS(&signature));
				if(signature) {
					ID3D12PipelineState *pipelinestate = nullptr;
					D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};

					pipeline_desc.pRootSignature        = signature;
					pipeline_desc.VS.pShaderBytecode    = &vscode[0];
					pipeline_desc.VS.BytecodeLength     = vscode.size();
					if(!gscode.empty()) {
						pipeline_desc.GS.pShaderBytecode = &gscode[0];
						pipeline_desc.GS.BytecodeLength = gscode.size();
					}
					pipeline_desc.PS.pShaderBytecode    = &pscode[0];
					pipeline_desc.PS.BytecodeLength     = pscode.size();
					pipeline_desc.InputLayout           = inputLayoutDesc;
					pipeline_desc.RasterizerState       = rasterizerState;
					pipeline_desc.DepthStencilState     = depthStencilState;
					pipeline_desc.SampleMask            = UINT_MAX;
					pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					pipeline_desc.NumRenderTargets      = MAX_MULTI_RENDER_TARGET_NUM;
					pipeline_desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
					pipeline_desc.SampleDesc.Count      = 1;
					for (int rtvcount = 0 ; rtvcount < MAX_MULTI_RENDER_TARGET_NUM; rtvcount++) {
						pipeline_desc.RTVFormats[rtvcount] = DXGI_FORMAT_R8G8B8A8_UNORM;
					}
					pipeline_desc.BlendState.AlphaToCoverageEnable = FALSE;
					pipeline_desc.BlendState.IndependentBlendEnable = FALSE;
					for (UINT rtvcount = 0; rtvcount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; rtvcount++) {
						pipeline_desc.BlendState.RenderTarget[rtvcount]          = defaultBlendDesc;
					}
					HRESULT status = device->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&pipelinestate));
					if(FAILED(status)) {
						printf("ID3D12PipelineState : cant create filename=%s\n", shader_filename);
					} else {
						root_signatures[i] = signature;
						pipeline_states[i] = pipelinestate;
					}
				}
			}
			RELEASE(blobsig);
		}
	}

	Device(HWND hWnd) {
		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		swap_chain_desc.BufferCount          = MAX_BACKBUFFER;
		swap_chain_desc.BufferDesc.Width     = Width;
		swap_chain_desc.BufferDesc.Height    = Height;
		swap_chain_desc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swap_chain_desc.SampleDesc.Count     = 1;
		swap_chain_desc.Windowed             = TRUE;
		D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};

		D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device));
		//D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
		device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&queue));

		//misc handles
		D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc = {};
		desc_heap_desc.NumDescriptors = MAX_RTV_HEAP;
		desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&rtv_descripter_heap));
		for(int i = 0 ; i < MAX_RTV_HEAP; i++) {
			auto handle = rtv_descripter_heap->GetCPUDescriptorHandleForHeapStart();
			auto handle_gpu = rtv_descripter_heap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			handle_gpu.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			rtv_descripter_handle[i] = handle;
			rtv_gpu_descripter_handle[i] = handle_gpu;
		}

		desc_heap_desc.NumDescriptors = MAX_DSV_HEAP;
		desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&dsv_descripter_heap));
		for(int i = 0 ; i < MAX_DSV_HEAP; i++) {
			auto handle = dsv_descripter_heap->GetCPUDescriptorHandleForHeapStart();
			auto handle_gpu = dsv_descripter_heap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			handle_gpu.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			dsv_descripter_handle[i] = handle;
			dsv_gpu_descripter_handle[i] = handle_gpu;
		}

		desc_heap_desc.NumDescriptors = MAX_CBV_HEAP;
		desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&cbv_descripter_heap));
		for(int i = 0 ; i < MAX_CBV_HEAP; i++) {
			auto handle = cbv_descripter_heap->GetCPUDescriptorHandleForHeapStart();
			auto handle_gpu = cbv_descripter_heap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			handle_gpu.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			cbv_descripter_handle[i] = handle;
			cbv_gpu_descripter_handle[i] = handle_gpu;
		}

		desc_heap_desc.NumDescriptors = MAX_SRV_HEAP;
		desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&srv_descripter_heap));
		for(int i = 0 ; i < MAX_SRV_HEAP; i++) {
			auto handle = srv_descripter_heap->GetCPUDescriptorHandleForHeapStart();
			auto handle_gpu = srv_descripter_heap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			handle_gpu.ptr += i * device->GetDescriptorHandleIncrementSize(desc_heap_desc.Type);
			srv_descripter_handle[i] = handle;
			srv_gpu_descripter_handle[i] = handle_gpu;
		}
		
		//SwapChain
		CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		IDXGISwapChain *swapchain_temp = nullptr;
		swap_chain_desc.OutputWindow         = hWnd;
		factory->CreateSwapChain(queue, &swap_chain_desc, &swapchain_temp);
		swapchain_temp->QueryInterface( IID_PPV_ARGS( &swapchain ) );
		factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
		RELEASE(swapchain_temp);

		//BackBuffers
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		for(int i = 0 ; i < MAX_BACKBUFFER; i++) {
			ID3D12Resource *backbuffer = nullptr;
			swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer));
			rtv_resources[i] = backbuffer;
			dsv_resources[i] = CreateCommittedResource(Width, Height, dsvDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			device->CreateRenderTargetView(rtv_resources[i], &rtvDesc, GetRTVHandle(i));
			device->CreateDepthStencilView(dsv_resources[i], &dsvDesc, GetDSVHandle(i));
		}

		//Default Render Target
		rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = rtvDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = MAX_MIPLEVELS;
		for(int i = MAX_BACKBUFFER ; i < MAX_RENDER_TARGET; i++) {
			rtv_resources[i] = CreateCommittedResource(Width, Height, rtvDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			dsv_resources[i] = CreateCommittedResource(Width, Height, dsvDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			device->CreateRenderTargetView(rtv_resources[i], &rtvDesc,   GetRTVHandle(i));
			device->CreateShaderResourceView(rtv_resources[i], &srvDesc, GetSRVHandle(i));
			device->CreateDepthStencilView(dsv_resources[i], &dsvDesc,   GetDSVHandle(i));
		}

		//Startup
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&commandlist) );
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		commandlist->Close();
		fenceValue = WaitForSignalCommandComplete(queue, fence, fenceEvent, fenceValue);

		//alloc vertex buffers
		for(int i = 0 ; i < MAX_VERTEX_BUFFER; i++) {
			auto size = MAX_VERTEX_COUNT * sizeof(VertexFormat);
			auto resource = CreateCommittedResource(size);
			vertex_resources[i] = resource;
		}

		//alloc constant buffers
		for(int i = 0 ; i < MAX_CONSTANT_BUFFER; i++) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC view = {};
			const size_t SizeInBytes = (MAX_CONSTANT_BUFFER_SIZE + 255) & ~255;
			auto resource = CreateCommittedResource(SizeInBytes);
			view.SizeInBytes = SizeInBytes;
			view.BufferLocation  = resource->GetGPUVirtualAddress();
			device->CreateConstantBufferView(&view, GetCBVHandle(i));
			cbv_resources[i] = resource;
			constant_views[i] = view;
		}

		ReloadPipeline();
	}

	void Begin(int *rtvIndices, size_t rtvCount, int dsvindex, float *clear_color, bool is_render_target = false) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtvColorHandle[MAX_MULTI_RENDER_TARGET_NUM] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDepthHandle = {};
		D3D12_VIEWPORT viewport = {0.0f,0.0f,Width,Height,0.0f,1.0f,};
		D3D12_RECT rect = {0, 0, Width, Height};
		for(int i = 0 ; i < rtvCount; i++) {
			int index = rtvIndices[i];
			rtvColorHandle[i] = GetRTVHandle(index);
			if(is_render_target) {
				BeginBarrier[i] = ResourceBarrier(rtv_resources[index], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
				EndBarrier[i] = ResourceBarrier(rtv_resources[index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
			} else {
				BeginBarrier[i] = ResourceBarrier(rtv_resources[index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				EndBarrier[i] = ResourceBarrier(rtv_resources[index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			}
		}
		rtvDepthHandle = GetDSVHandle(dsvindex);

		allocator->Reset();
		commandlist->Reset(allocator, 0);
		commandlist->RSSetViewports(1, &viewport);
		commandlist->RSSetScissorRects(1, &rect);
		commandlist->ResourceBarrier(rtvCount, BeginBarrier);
		commandlist->OMSetRenderTargets(rtvCount, rtvColorHandle, FALSE, &rtvDepthHandle);
		for (int i = 0 ; i < rtvCount; i++) {
			commandlist->ClearRenderTargetView(rtvColorHandle[i], clear_color, 0, NULL);
		}
		commandlist->ClearDepthStencilView(rtvDepthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
		commandlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	bool SetEffect(int index) {
		if(pipeline_states[index]) {
			commandlist->SetPipelineState(pipeline_states[index]);
			commandlist->SetGraphicsRootSignature(root_signatures[index]);
			return true;
		}
		return false;
	}

	void SetVertex(int index) {
		D3D12_VERTEX_BUFFER_VIEW vb_view = vertex_views[index];
		commandlist->IASetVertexBuffers(0, 1, &vb_view);
	}

	void SetConstant(int tableindex, int index) {
		commandlist->SetDescriptorHeaps(1, &cbv_descripter_heap);
		commandlist->SetGraphicsRootDescriptorTable(tableindex, GetGPUCBVHandle(index));
	}

	void SetTexture(int tableindex, int index) {
		commandlist->SetDescriptorHeaps(1, &srv_descripter_heap);
		commandlist->SetGraphicsRootDescriptorTable(tableindex, GetGPUSRVHandle(index));
	}

	void End(int rtvCount) {
		commandlist->ResourceBarrier(rtvCount, EndBarrier);
		commandlist->Close();
		ID3D12CommandList *ppLists[] = {
			commandlist,
		};
		queue->ExecuteCommandLists(1, ppLists);
		fenceValue = WaitForSignalCommandComplete(queue, fence, fenceEvent, fenceValue);
	}
	
	void Present() {
		swapchain->Present(1, 0);
	}

	void UploadConstant(int index, void *data, size_t size) {
		auto res = cbv_resources[index];
		UploadResourceSubData(res, data, size);
	}

	void UploadVertex(int index, void *data, size_t size,  size_t stride_size) {
		D3D12_VERTEX_BUFFER_VIEW view = {};
		auto resource = vertex_resources[index];
		view.StrideInBytes = stride_size;
		view.SizeInBytes = size;
		view.BufferLocation = resource->GetGPUVirtualAddress();
		UploadResourceSubData(resource, data, size);
		vertex_views[index] = view;
	}
};

