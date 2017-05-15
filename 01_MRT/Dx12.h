#include <d3d12.h>
#include <pix.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>

#ifndef RELEASE
#define RELEASE(x) {if(x) {x->Release(); x = NULL; } }
#endif


namespace Dx12 {
typedef std::vector<unsigned char> D3DShaderVector;

inline void CompileShaderFromFile(D3DShaderVector &vdest, const char *filename, const char *entryname, const char *profile, UINT flags, ID3DBlob **blobsig = 0)
{
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
		printf("\n===============================================\nCOMPILE ERROR\n");
		if (blobError) {
			printf("%s\n", blobError->GetBufferPointer());
		}
		printf("\n===============================================\n\n");
	}

	//https://glhub.blogspot.jp/2016/08/dx12-hlslroot-signature.html
	if (blob && blobsig) {
		if (S_OK == D3DGetBlobPart(blob->GetBufferPointer(), blob->GetBufferSize(), D3D_BLOB_ROOT_SIGNATURE, 0, blobsig))
		{
		}
	}
	RELEASE(blob);
}


inline D3D12_RASTERIZER_DESC CreateRasterizerDesc(int mode) {
	D3D12_RASTERIZER_DESC desc = {};
	desc.FillMode              = D3D12_FILL_MODE_SOLID;
	desc.CullMode              = D3D12_CULL_MODE_BACK;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable       = TRUE;
	desc.MultisampleEnable     = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount     = 0;
	desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return desc;
}

inline D3D12_BLEND_DESC CreateBlendDesc(int mode) {
	D3D12_RENDER_TARGET_BLEND_DESC defaultDesc = {};
	//defaultDesc.BlendEnable           = TRUE;
	printf("Warning: BlendEnable=FALSE\n");
	defaultDesc.BlendEnable           = FALSE;
	defaultDesc.LogicOpEnable         = FALSE;

	defaultDesc.SrcBlend              = D3D12_BLEND_SRC_ALPHA ;
	//defaultDesc.DestBlend             = D3D12_BLEND_ONE     ; //D3D12_BLEND_INV_DEST_ALPHA
	defaultDesc.DestBlend             = D3D12_BLEND_INV_DEST_ALPHA;
	defaultDesc.BlendOp               = D3D12_BLEND_OP_ADD;
	defaultDesc.SrcBlendAlpha         = D3D12_BLEND_ONE;
	defaultDesc.DestBlendAlpha        = D3D12_BLEND_ZERO;
	defaultDesc.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
	defaultDesc.LogicOp               = D3D12_LOGIC_OP_NOOP;
	defaultDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_BLEND_DESC desc = {};
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
		desc.RenderTarget[i] = defaultDesc;
	}
	return desc;
}

inline D3D12_SHADER_RESOURCE_VIEW_DESC CreateShaderResourceViewDesc(DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension, UINT Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	memset(&desc, 0, sizeof(desc));
	desc.Format                        = Format;
	desc.ViewDimension                 = ViewDimension;
	desc.Shader4ComponentMapping       = Shader4ComponentMapping;
	desc.Texture2D.MipLevels           = 1;
	desc.Texture2D.MostDetailedMip     = 0;
	desc.Texture2D.PlaneSlice          = 0;
	desc.Texture2D.ResourceMinLODClamp = 0;
	return desc;
}

inline D3D12_INPUT_LAYOUT_DESC CreateInputLayoutDesc(int mode) {
	static const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV",       0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	D3D12_INPUT_LAYOUT_DESC desc = {};
	desc.pInputElementDescs      = inputElementDescs;
	desc.NumElements             = _countof(inputElementDescs);
	return desc;
}


inline ID3D12Device *CreateDevice() {
	ID3D12Device *ret = NULL;
	//D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&ret));
	D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ret));
	return ret;
}

inline ID3D12CommandQueue *CreateCommandQueue(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type)
{
	if (!device) {
		printf("ERROR NULL DEVICE %s\n", __FUNCTION__);
		return NULL;
	}
	ID3D12CommandQueue *ret       = NULL;
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.Type                     = type;
	device->CreateCommandQueue(&desc, IID_PPV_ARGS(&ret));
	return ret;
}

inline IDXGISwapChain3 *CreateSwapChain3(ID3D12Device *device, ID3D12CommandQueue *queue, HWND hWnd, int width, int height, int framecount)
{
	if (!device) {
		printf("ERROR NULL DEVICE %s\n", __FUNCTION__);
		return NULL;
	}
	IDXGISwapChain3 *ret      = NULL;
	IDXGIFactory4 *factory    = NULL;

	CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferCount          = framecount;
	desc.BufferDesc.Width     = width;
	desc.BufferDesc.Height    = height;
	desc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.OutputWindow         = hWnd;
	desc.SampleDesc.Count     = 1;
	desc.Windowed             = TRUE;

	IDXGISwapChain *m_swapChainRef = NULL;
	factory->CreateSwapChain(queue, &desc, &m_swapChainRef);
	m_swapChainRef->QueryInterface( IID_PPV_ARGS( &ret ) );
	factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	RELEASE(m_swapChainRef);
	return ret;
}

inline ID3D12DescriptorHeap *CreateDescriptorHeap(ID3D12Device *device, int num, D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
{
	if (!device) {
		printf("ERROR NULL DEVICE %s\n", __FUNCTION__);
		return NULL;
	}
	ID3D12DescriptorHeap *ret       = NULL;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors             = num;
	desc.Type                       = type;
	desc.Flags                      = flag;

	//https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn859378(v=vs.85).aspx
	//This flag only applies to CBV, SRV, UAV and samplers.
	//It does not apply to other descriptor heap types since shaders do not directly reference the other types.
	if (type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && (flag | D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)) {
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ret));
	return ret;
}

inline void CreateRenderTargetView(ID3D12Device *device, ID3D12Resource *rendertarget, D3D12_RENDER_TARGET_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	device->CreateRenderTargetView(rendertarget, desc, handle);
}

inline void CreateDepthStencilView(ID3D12Device *device, ID3D12Resource *rendertarget, D3D12_DEPTH_STENCIL_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	device->CreateDepthStencilView(rendertarget, desc, handle);
}

inline void CreateShaderResourceView(ID3D12Device *device, ID3D12Resource *rendertarget, D3D12_SHADER_RESOURCE_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	device->CreateShaderResourceView(rendertarget, desc, handle);
}

inline ID3D12Resource *GetSwapChainRenderTarget(IDXGISwapChain3 *swapChain, UINT index) {
	ID3D12Resource *ret = 0;
	if (swapChain) {
		swapChain->GetBuffer(index, IID_PPV_ARGS(&ret));
	}
	return ret;
}

inline D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle(ID3D12Device *device, ID3D12DescriptorHeap *heap, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index = 0)
{
	D3D12_CPU_DESCRIPTOR_HANDLE ret = heap->GetCPUDescriptorHandleForHeapStart();
	ret.ptr += device->GetDescriptorHandleIncrementSize(type) * index;
	return ret;
}

inline ID3D12CommandAllocator *CreateCommandAllocator(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandAllocator *ret = NULL;
	device->CreateCommandAllocator(type, IID_PPV_ARGS(&ret));
	return ret;
}

inline ID3D12RootSignature *CreateRootSignature(ID3D12Device *device)
{
	ID3D12RootSignature *ret                  = NULL;
	ID3DBlob            *signature            = NULL;
	ID3DBlob            *error                = NULL;

	//RANGE
	enum {
		MAX_DESC = 1,
	};
	const D3D12_DESCRIPTOR_RANGE_TYPE RangeTypes[] = {
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
	};

	//SRV : tX : 16
	//CBV : bX : 16
	const int NumDescriptors[] = {
		4, 4
	};
	//シェーダーリソースと定数しか使わぬ
	D3D12_DESCRIPTOR_RANGE range[MAX_DESC] = {};
	D3D12_ROOT_PARAMETER param[MAX_DESC] = {};

	for (int i = 0 ; i < MAX_DESC; i++)
	{
		range[i].RangeType                           = RangeTypes[i];
		range[i].NumDescriptors                      = NumDescriptors[i];
		range[i].BaseShaderRegister                  = 0;
		range[i].RegisterSpace                       = 0;
		range[i].OffsetInDescriptorsFromTableStart   = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		param[i].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[i].DescriptorTable.NumDescriptorRanges = MAX_DESC;
		param[i].DescriptorTable.pDescriptorRanges   = &range[i];
		param[i].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

	}

	//sampler
	D3D12_STATIC_SAMPLER_DESC sampler_temp = {};
	sampler_temp.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_temp.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_temp.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_temp.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_temp.MipLODBias       = 0;
	sampler_temp.MaxAnisotropy    = 0;
	sampler_temp.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
	sampler_temp.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_temp.MinLOD           = 0.0f;
	sampler_temp.MaxLOD           = D3D12_FLOAT32_MAX;
	sampler_temp.ShaderRegister   = 0;
	sampler_temp.RegisterSpace    = 0;
	sampler_temp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	D3D12_STATIC_SAMPLER_DESC samplers[] = {
		sampler_temp,
	};

	D3D12_ROOT_SIGNATURE_DESC  desc           = {};
	desc.NumParameters                        = _countof(param);
	desc.pParameters                          = param;
	desc.NumStaticSamplers                    = 1;
	desc.pStaticSamplers                      = samplers;
	desc.Flags                                = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	HRESULT hRet = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(hRet)) {
		printf("ERROR : Cant D3D12SerializeRootSignature\n");
		return NULL;
	}

	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&ret));
	if (!ret) {
		printf("ERROR : Cant CreateRootSignature\n");
	}
	RELEASE(signature);
	RELEASE(error    );
	return ret;
}

inline ID3D12RootSignature *CreateRootSignature2(ID3D12Device *device)
{
	ID3D12RootSignature *ret                  = NULL;
	ID3DBlob            *signature            = NULL;
	ID3DBlob            *error                = NULL;

	//RANGE
	enum {
		OFFSET_SRV = 0,
		OFFSET_CBV = 1,
		OFFSET_MAX = 3,
	};

	D3D12_DESCRIPTOR_RANGE range[OFFSET_MAX] = {};
	D3D12_ROOT_PARAMETER param[OFFSET_MAX] = {};

	//SRV
	int offset = 0;
	for (int i = offset ; i < OFFSET_CBV; i++)
	{
		range[i].RangeType                           = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range[i].NumDescriptors                      = 1;
		range[i].BaseShaderRegister                  = 0;
		range[i].RegisterSpace                       = 0;
		range[i].OffsetInDescriptorsFromTableStart   = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		param[i].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[i].DescriptorTable.NumDescriptorRanges = 1;
		param[i].DescriptorTable.pDescriptorRanges   = &range[i];
		param[i].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

		offset++;
	}

	//CBV
	for (int i = offset ; i < OFFSET_MAX; i++)
	{
		range[i].RangeType                           = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		range[i].NumDescriptors                      = 1;
		range[i].BaseShaderRegister                  = 0;
		range[i].RegisterSpace                       = 0;
		range[i].OffsetInDescriptorsFromTableStart   = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		param[i].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[i].DescriptorTable.NumDescriptorRanges = 1;
		param[i].DescriptorTable.pDescriptorRanges   = &range[i];
		param[i].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

		offset++;
	}

	//sampler
	D3D12_STATIC_SAMPLER_DESC sampler_temp = {};
	sampler_temp.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_temp.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_temp.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_temp.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_temp.MipLODBias       = 0;
	sampler_temp.MaxAnisotropy    = 0;
	sampler_temp.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
	sampler_temp.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_temp.MinLOD           = 0.0f;
	sampler_temp.MaxLOD           = D3D12_FLOAT32_MAX;
	sampler_temp.ShaderRegister   = 0;
	sampler_temp.RegisterSpace    = 0;
	sampler_temp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	D3D12_STATIC_SAMPLER_DESC samplers[] = {
		sampler_temp,
	};

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(param);
	desc.pParameters = param;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = samplers;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	HRESULT hRet = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(hRet)) {
		printf("ERROR : Cant D3D12SerializeRootSignature\n");
		return NULL;
	}

	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&ret));
	if (!ret) {
		printf("ERROR : Cant CreateRootSignature\n");
	}
	RELEASE(signature);
	RELEASE(error    );
	return ret;
}

inline ID3D12PipelineState *CreateGraphicsPipelineState(
    ID3D12Device *device,
    ID3D12RootSignature *rootsignature,
    int rtvcount,
    DXGI_FORMAT rtvformat,
    DXGI_FORMAT depthformat,
    int blendmode,
    int rasterizemode,
    int layoutmode,
    D3DShaderVector *vshader,
    D3DShaderVector *gshader,
    D3DShaderVector *pshader)
{
	if (!device) {
		printf("ERROR NULL DEVICE %s\n", __FUNCTION__);
		return NULL;
	}
	D3D12_SHADER_BYTECODE VS = {0, 0};
	D3D12_SHADER_BYTECODE GS = {0, 0};
	D3D12_SHADER_BYTECODE PS = {0, 0};
	if (vshader) {
		VS.pShaderBytecode = &(*vshader)[0];
		VS.BytecodeLength = vshader->size();
	}
	if (gshader) {
		GS.pShaderBytecode = &(*gshader)[0];
		GS.BytecodeLength = gshader->size();
	}

	if (pshader) {
		PS.pShaderBytecode = &(*pshader)[0];
		PS.BytecodeLength = pshader->size();
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout                     = CreateInputLayoutDesc(layoutmode);
	desc.pRootSignature                  = rootsignature;
	desc.VS                              = VS;
	desc.GS                              = GS;
	desc.PS                              = PS;

	desc.RasterizerState                 = CreateRasterizerDesc(rasterizemode);
	desc.BlendState                      = CreateBlendDesc(blendmode);

	//https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn770356(v=vs.85).aspx
	desc.DepthStencilState.DepthEnable      = TRUE;
	desc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.StencilEnable    = FALSE;
	desc.DepthStencilState.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	desc.DepthStencilState.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
	desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	desc.DepthStencilState.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
	desc.DepthStencilState.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.DepthStencilState.BackFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
	desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	desc.DepthStencilState.BackFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
	desc.DepthStencilState.BackFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.SampleMask                      = UINT_MAX;
	desc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets                = rtvcount;
	for (int i = 0 ; i < rtvcount; i++) {
		desc.RTVFormats[i]               = rtvformat;
	}
	desc.DSVFormat                       = depthformat;
	desc.SampleDesc.Count                = 1;

	ID3D12PipelineState *ret = NULL;
	HRESULT status = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&ret));
	if (FAILED(status)) {
		printf("FAILED : CreateGraphicsPipelineState reason=%08X\n", status);
	} else {
		printf("OK! : CreateGraphicsPipelineState\n");
	}
	return ret;
}

inline ID3D12GraphicsCommandList *CreateCommandList(
    ID3D12Device *device,
    ID3D12CommandAllocator *allocator,
    D3D12_COMMAND_LIST_TYPE type,
    ID3D12PipelineState *pipelinestate = NULL)
{
	ID3D12GraphicsCommandList *ret = NULL;
	device->CreateCommandList(0, type, allocator, pipelinestate, IID_PPV_ARGS(&ret) );
	return ret;
}

inline D3D12_HEAP_PROPERTIES CreateHeapProperties(D3D12_HEAP_TYPE type)
{
	D3D12_HEAP_PROPERTIES ret = {};
	ret.Type                 = type;
	ret.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	ret.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; //FOR L1, L2
	ret.CreationNodeMask     = 1;
	ret.VisibleNodeMask      = 1;
	return ret;
}


inline D3D12_RESOURCE_DESC CreateResourceDesc(
    D3D12_RESOURCE_DIMENSION dimension,
    UINT64 width,
    UINT64 height = 1,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension          = dimension;
	desc.Alignment          = 0;
	desc.Width              = width;
	desc.Height             = height;
	desc.DepthOrArraySize   = 1;
	desc.MipLevels          = 1;
	desc.Format             = format;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags              = flag;
	if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
		//https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn903813(v=vs.85).aspx
		//Height, DepthOrArraySize, and MipLevels must be 1.
		//SampleDesc.Count must be 1 and Quality must be 0.
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.DepthOrArraySize   = 1;
		desc.MipLevels          = 1;
		desc.SampleDesc.Count   = 1;
		desc.SampleDesc.Quality = 0;
	}
	return desc;
}

inline ID3D12Resource *CreateCommittedResource(
    ID3D12Device *device,
    UINT width,
    UINT height = 1,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE,
    D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_GENERIC_READ)
{
	ID3D12Resource *ret = NULL;
	D3D12_HEAP_PROPERTIES heap_prop = CreateHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	//cant use 2D texture.mendokusai.
	if (height >= 2) {
		dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		heap_prop = CreateHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	}

	D3D12_RESOURCE_DESC resouce_desc = CreateResourceDesc(dimension, width, height, format, flag);
	device->CreateCommittedResource(
	    &heap_prop,
	    D3D12_HEAP_FLAG_NONE,
	    &resouce_desc,
	    resource_state,
	    0,
	    IID_PPV_ARGS(&ret));
	return ret;
}

inline void GetCopyableFootprints(
    ID3D12Device *device,
    const D3D12_RESOURCE_DESC *desc,
    UINT first_res,
    UINT num_res,
    UINT64 offset,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
    UINT *pNumRows,
    UINT64 *pRowSizeInBytes,
    UINT64 *pTotalBytes)
{
	device->GetCopyableFootprints(
	    desc,
	    first_res,
	    num_res,
	    offset,
	    pLayouts,
	    pNumRows,
	    pRowSizeInBytes,
	    pTotalBytes);
}

inline void CopyResourceSubData(ID3D12Resource *resource, void *data, UINT size)
{
	UINT8 *pData = NULL;
	resource->Map(0, NULL, reinterpret_cast<void**>(&pData));
	if (pData) {
		memcpy(pData, data, size);
		resource->Unmap(0, NULL);
	} else {
		printf("Failed Map\n");
	}
}

inline ID3D12Fence *CreateFence(ID3D12Device *device)
{
	ID3D12Fence *ret = NULL;
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ret));
	return ret;
}

inline D3D12_SUBRESOURCE_DATA CreateSubResourceData(const void *pData, LONG_PTR RowPitch, LONG_PTR SlicePitch)
{
	D3D12_SUBRESOURCE_DATA ret = {};
	ret.pData = pData ;
	ret.RowPitch = RowPitch ;
	ret.SlicePitch = SlicePitch;
	return ret;
}

inline D3D12_RESOURCE_BARRIER ResourceBarrier(
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

inline void WaitForSignalCommandComplete(ID3D12CommandQueue *queue, ID3D12Fence *fence, HANDLE fenceEvent, UINT64 &fenceValue)
{
	const UINT64 fenceSignalValue = fenceValue;
	queue->Signal(fence, fenceSignalValue);
	fenceValue++;
	if (fence->GetCompletedValue() < fenceSignalValue)
	{
		fence->SetEventOnCompletion(fenceSignalValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

inline DWORD WaitForSignalSwapChain(IDXGISwapChain3 *swapChain, ID3D12CommandQueue *queue, ID3D12Fence *fence, HANDLE fenceEvent, UINT64 &fenceValue)
{
	WaitForSignalCommandComplete(queue, fence, fenceEvent, fenceValue);
	return swapChain->GetCurrentBackBufferIndex();
}

inline D3D12_VIEWPORT CreateViewPort(float x, float y, float w, float h, float mindepth, float maxdepth)
{
	D3D12_VIEWPORT ret;
	ret.TopLeftX = x;
	ret.TopLeftY = y;
	ret.Width    = w;
	ret.Height   = h;
	ret.MinDepth = mindepth;
	ret.MaxDepth = maxdepth;
	return ret;
}

// https://glhub.blogspot.jp/2016/07/dx12-getcopyablefootprints.html
inline ID3D12Resource *UploadResource(
    ID3D12Device *device, ID3D12GraphicsCommandList *cmdlist, ID3D12Resource *textureBuffer, void * buffer)
{
	UINT64 uploadSize = 0;
	UINT64 rowSizeInBytes = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	const D3D12_RESOURCE_DESC destDesc = textureBuffer->GetDesc();
	GetCopyableFootprints(
	    device,
	    &destDesc,
	    0,
	    1,
	    0,
	    &footprint,
	    0,
	    &rowSizeInBytes,
	    &uploadSize);
	uploadSize = (uploadSize + 255) & ~255;
	if (rowSizeInBytes != footprint.Footprint.RowPitch) {
		printf("%s : Error : rowSizeInBytes != footprint.Footprint.RowPitch, %d %d\n", __FUNCTION__, rowSizeInBytes, footprint.Footprint.RowPitch);
		return NULL;
	}
	ID3D12Resource *uploadBuf = CreateCommittedResource(
	                                device,
	                                uploadSize);
	if (!uploadBuf) {
		printf("%s : uploadBuf cant alloc!\n", __FUNCTION__);
		return NULL;
	}
	CopyResourceSubData(uploadBuf, buffer, uploadSize);


	D3D12_TEXTURE_COPY_LOCATION srcBufLocation = {
		uploadBuf,
		D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		footprint
	};
	D3D12_TEXTURE_COPY_LOCATION destBufLocation = {
		textureBuffer,
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		0
	};

	D3D12_RESOURCE_BARRIER transBarrier = ResourceBarrier(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	cmdlist->CopyTextureRegion(&destBufLocation, 0, 0, 0, &srcBufLocation, nullptr);
	cmdlist->ResourceBarrier(1, &transBarrier);
	return uploadBuf;
}


class Resource {
protected:
	struct {
		ID3D12Resource *resource;
		union {
			ID3D12DescriptorHeap *heap;
			struct {
				ID3D12RootSignature *signature;
				ID3D12PipelineState *pipelinestate;
			};
		};
	};
	union {
		D3D12_VERTEX_BUFFER_VIEW VertexView;
		D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantView;
		D3D12_SHADER_RESOURCE_VIEW_DESC ShaderView;
	};
public:
	Resource() {
		memset(&VertexView, 0, sizeof(VertexView));
		memset(&ConstantView, 0, sizeof(ConstantView));
		memset(&ShaderView, 0, sizeof(ShaderView));
		resource = 0;
		heap = 0;
		signature = 0;
		pipelinestate = 0;
	}
	virtual ~Resource() {
		RELEASE(heap);
		RELEASE(pipelinestate);
		RELEASE(signature);
		RELEASE(resource);
	}
	ID3D12Resource *Get() {
		return resource;
	}
	ID3D12DescriptorHeap *GetHeap() {
		return heap;
	}
	ID3D12RootSignature *GetRootSignature() {
		return signature;
	}
	ID3D12PipelineState *GetPipelineState() {
		return pipelinestate;
	}

	template <typename T>
	T GetView() {};

	template <>
	D3D12_VERTEX_BUFFER_VIEW GetView<D3D12_VERTEX_BUFFER_VIEW>() {
		return VertexView;
	}

	template <>
	D3D12_CONSTANT_BUFFER_VIEW_DESC GetView<D3D12_CONSTANT_BUFFER_VIEW_DESC>() {
		return ConstantView;
	}

	template <>
	D3D12_SHADER_RESOURCE_VIEW_DESC GetView<D3D12_SHADER_RESOURCE_VIEW_DESC>() {
		return ShaderView;
	}

	void *Map() {
		void *pData = NULL;
		if (resource) {
			resource->Map(0, NULL, reinterpret_cast<void**>(&pData));
		}
		return pData;
	}

	void Unmap() {
		if (resource) {
			resource->Unmap(0, NULL);
		}
	}
};

class Vertex : public Resource {
public:
	Vertex(ID3D12Device *device, void *buffer, size_t size, size_t stride_size) {
		resource = CreateCommittedResource(device, size);
		if (buffer) {
			CopyResourceSubData(resource, buffer, size);
		}
		VertexView.StrideInBytes = stride_size;
		VertexView.SizeInBytes = size;
		VertexView.BufferLocation = resource->GetGPUVirtualAddress();
		printf("%s : Create : size=%d, StrideInBytes=%d\n", __FUNCTION__, size, stride_size);
	}
};

class Constant : public Resource {
public:
	Constant(ID3D12Device *device, size_t size) {
		const size_t SizeInBytes = (size + 255) & ~255;
		resource = CreateCommittedResource(device, SizeInBytes);
		heap = CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		ConstantView.SizeInBytes = SizeInBytes;
		ConstantView.BufferLocation  = resource->GetGPUVirtualAddress();
		device->CreateConstantBufferView(&ConstantView, heap->GetCPUDescriptorHandleForHeapStart());
	}
};


class Shader : public Resource {
	D3DShaderVector vscode;
	D3DShaderVector pscode;
public:
	Shader(ID3D12Device *device, const char *filename, bool is_rendertarget, int rtvcount, int blendmode = 0, int rasterizemode = 0, int layoutmode = 0) {
		ID3DBlob *blobsig = NULL;

		CompileShaderFromFile(vscode, filename, "VSMain", "vs_5_0", 0, &blobsig);
		CompileShaderFromFile(pscode, filename, "PSMain", "ps_5_0", 0);
		if (blobsig) {

			device->CreateRootSignature(0, blobsig->GetBufferPointer(), blobsig->GetBufferSize(), IID_PPV_ARGS(&signature));
			RELEASE(blobsig);

			if (!signature) {
				//signature  = CreateRootSignature(device);
			} else {
				//Create Signature from hlsl
			}
			pipelinestate = CreateGraphicsPipelineState(
			                    device,
			                    signature,
			                    rtvcount,
			                    is_rendertarget ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM, //todo
			                    DXGI_FORMAT_D32_FLOAT,
			                    blendmode,
			                    rasterizemode,
			                    layoutmode,
			                    &vscode,
			                    nullptr,
			                    &pscode);
			if (!pipelinestate) {
				printf("ERROR : %s : CreateGraphicsPipelineState\n", __FUNCTION__);
			} else {
				printf("%s : OK filename=%s\n", __FUNCTION__, filename);
			}
		}
	}
};

class Texture : public Resource {
public:
	Texture(ID3D12Device *device, int w, int h, bool is_rendertarget = false, bool is_depth = false) {
		auto format = DXGI_FORMAT_R8G8B8A8_UNORM;
		auto flags = D3D12_RESOURCE_FLAG_NONE;
		auto state = D3D12_RESOURCE_STATE_COMMON;
		if (is_rendertarget) {
			format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			state = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		if (is_depth) {
			format = DXGI_FORMAT_D32_FLOAT;
			flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		/*
		//diag
		if( format  == DXGI_FORMAT_R8G8B8A8_UNORM ) printf("DXGI_FORMAT_R8G8B8A8_UNORM \n");;
		if( flags   == D3D12_RESOURCE_FLAG_NONE   ) printf("D3D12_RESOURCE_FLAG_NONE   \n");;
		if( state   == D3D12_RESOURCE_STATE_COMMON) printf("D3D12_RESOURCE_STATE_COMMON\n");;
		if( format  == DXGI_FORMAT_R32G32B32A32_FLOAT         ) printf("DXGI_FORMAT_R32G32B32A32_FLOAT         \n");;
		if( flags   == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) printf("D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET\n");;
		if( state   == D3D12_RESOURCE_STATE_GENERIC_READ      ) printf("D3D12_RESOURCE_STATE_GENERIC_READ      \n");;
		*/

		printf("%s : INFO : %d %d\n", __FUNCTION__, w, h);
		resource = CreateCommittedResource(device, w, h, format, flags, state);
		if (!resource) {
			printf("ERROR : %s : Cant Create RES\n", __FUNCTION__);
		}
		ShaderView = CreateShaderResourceViewDesc(format, D3D12_SRV_DIMENSION_TEXTURE2D);
		heap = CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		if (!is_depth) {
			CreateShaderResourceView(device, resource, &ShaderView, heap->GetCPUDescriptorHandleForHeapStart());
		}
		printf("Texture Heap=%X\n", heap);
	}
};

class RenderTarget : public Resource {
	enum {
		MAX = 8,
	};

	ID3D12DescriptorHeap *rtvColorHeap = 0;
	ID3D12DescriptorHeap *rtvDepthHeap = 0;
	D3D12_RENDER_TARGET_VIEW_DESC rtvColorDesc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC rtvDepthDesc = {};
	Texture *color_texture[MAX] = {};
	Texture *depth_texture = 0;

	int Width;
	int Height;
	int Count;
	int PresentCount;
public:
	ID3D12DescriptorHeap *GetColorHeap() {
		return rtvColorHeap;
	}
	ID3D12DescriptorHeap *GetDepthHeap() {
		return rtvDepthHeap;
	}
	Texture *GetTexture(int index) {
		return color_texture[index];
	}
	Texture *GetDepth() {
		return depth_texture;
	}

	~RenderTarget() {
		for (int i = 0 ; i < MAX; i++) {
			if (color_texture[i]) delete color_texture[i];
		}
		if (depth_texture) delete depth_texture;
		RELEASE(rtvDepthHeap);
		RELEASE(rtvColorHeap);
	}

	RenderTarget(ID3D12Device *device, int width, int height, int count) {
		PresentCount      = 0;
		Count = count;

		//color
		rtvColorDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		rtvColorDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvColorDesc.Texture2D.MipSlice = 0;
		rtvColorDesc.Texture2D.PlaneSlice = 0;

		rtvColorHeap = CreateDescriptorHeap(device, Count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (int i = 0 ; i < Count; i++) {
			color_texture[i] = new Texture(device, width, height, true, false);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvColorHandle = CpuDescriptorHandle(device, rtvColorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i);
			CreateRenderTargetView(device, color_texture[i]->Get(), &rtvColorDesc, rtvColorHandle);
		}

		//DEPTH
		rtvDepthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		rtvDepthDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		rtvDepthDesc.Flags = D3D12_DSV_FLAG_NONE;
		//https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn770357(v=vs.85).aspx
		rtvDepthHeap = CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		depth_texture = new Texture(device, width, height, true, true);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDepthHandle = CpuDescriptorHandle(device, rtvDepthHeap, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0);
		CreateDepthStencilView(device, depth_texture->Get(), &rtvDepthDesc, rtvDepthHandle);
	}

};


class Device {
public:
	enum {
		RENDER = 0,
		UPLOAD,
		MAX,
	};
private:
	ID3D12Device *device = 0;
	IDXGISwapChain3 *swapChain = 0;

	ID3D12CommandQueue *cmdQueue[MAX] = {};
	ID3D12CommandAllocator *cmdAllocDirect[MAX] = {};
	ID3D12GraphicsCommandList *cmdListDirect[MAX] = {};

	UINT64 fenceValue[MAX] = {};
	ID3D12Fence *fence[MAX] = {};
	HANDLE fenceEvent[MAX] = {};

	std::vector<ID3D12Resource *> backBuffers;
	ID3D12Resource *backBufferDepth = 0;
	ID3D12DescriptorHeap *backBufferHeap = 0;
	ID3D12DescriptorHeap *depthBufferHeap = 0;
	D3D12_RENDER_TARGET_VIEW_DESC backbufferDesc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC depthBufferDesc = {};
	UINT FrameIndex = 0;
public:
	ID3D12Device *Get() {
		return device;
	}

	ID3D12CommandQueue *GetQueue(UINT idx = RENDER) {
		return cmdQueue[idx];
	}
	ID3D12CommandAllocator *GetAllocator(UINT idx = RENDER) {
		return cmdAllocDirect[idx];
	}
	ID3D12GraphicsCommandList *GetList(UINT idx = RENDER) {
		return cmdListDirect[idx];
	}

	IDXGISwapChain3 *GetSwapChain() {
		return swapChain;
	}

	ID3D12Resource *GetBackBuffer(int index) {
		return backBuffers[index];
	}
	ID3D12Resource *GetDepth(int index) {
		return backBufferDepth;
	}

	ID3D12DescriptorHeap *GetBackBufferHeap() {
		return backBufferHeap;
	}

	ID3D12DescriptorHeap *GetDepthBufferHeap() {
		return depthBufferHeap;
	}
	~Device() {
		for (int i = 0; i < backBuffers.size(); i++) {
			RELEASE(backBuffers[i]);
		}
		RELEASE(backBufferHeap);
		RELEASE(backBufferDepth);
		RELEASE(depthBufferHeap);

		for (int i = 0; i < MAX; i++) {
			if (fenceEvent[i]) CloseHandle(fenceEvent[i]);
			RELEASE(fence[i]);
			RELEASE(cmdListDirect[i]);
			RELEASE(cmdAllocDirect[i]);
			RELEASE(cmdQueue[i]);
		}

		RELEASE(device);
	}

	UINT GetFrameIndex() {
		return FrameIndex;
	}

	Device(HWND hWnd, UINT Width, UINT Height, UINT FrameCount) {
		device = CreateDevice();
		for (int i = 0 ; i < MAX; i++) {
			cmdQueue[i] = CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			cmdAllocDirect[i] = CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			cmdListDirect[i] = CreateCommandList(device, cmdAllocDirect[i], D3D12_COMMAND_LIST_TYPE_DIRECT);
			cmdListDirect[i]->Close();
			fence[i] = CreateFence(device);
			fenceEvent[i] = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		}

		swapChain = CreateSwapChain3(device, cmdQueue[RENDER], hWnd, Width, Height, FrameCount);

		//Create BackBuffer
		backbufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		backbufferDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		backbufferDesc.Texture2D.MipSlice = 0;
		backbufferDesc.Texture2D.PlaneSlice = 0;
		backBufferHeap = CreateDescriptorHeap(device, FrameCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (int i = 0 ; i < FrameCount; i++) {
			auto *resource = GetSwapChainRenderTarget(swapChain, i);
			backBuffers.push_back(resource);
			D3D12_CPU_DESCRIPTOR_HANDLE handle = CpuDescriptorHandle(device, backBufferHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i);
			CreateRenderTargetView(device, resource, &backbufferDesc, handle);
		}

		//DEPTH
		depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthBufferDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthBufferDesc.Flags = D3D12_DSV_FLAG_NONE;
		{
			//https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn770357(v=vs.85).aspx
			depthBufferHeap = CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvDepthHandle = CpuDescriptorHandle(device, depthBufferHeap, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0);
			backBufferDepth = CreateCommittedResource(device, Width, Height, depthBufferDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			CreateDepthStencilView(device, backBufferDepth, &depthBufferDesc, rtvDepthHandle);
		}

		//Initialize Fence
		for (int i = 0 ; i < MAX; i++) {
			WaitForCompleteExecute(i);
		}
	}

	void Execute(int idx = RENDER) {
		ID3D12CommandList *ppLists[] = {
			cmdListDirect[idx],
		};
		cmdQueue[idx]->ExecuteCommandLists(1, ppLists);
	}

	UINT SwapBuffer() {
		swapChain->Present(1, 0);
		UINT idx = RENDER;
		WaitForSignalCommandComplete(cmdQueue[idx], fence[idx], fenceEvent[idx], fenceValue[idx]);
		return swapChain->GetCurrentBackBufferIndex();
	}

	void WaitForCompleteExecute(UINT idx = RENDER) {
		WaitForSignalCommandComplete(cmdQueue[idx], fence[idx], fenceEvent[idx], fenceValue[idx]);
	}
};




class Renderer {
	std::map<std::string, BaseBuffer *> buffermapper;

	Device *device = 0;
	std::map<std::string, Constant *> cache_constant;
	std::map<std::string, Vertex *> cache_vertex;
	std::map<std::string, Shader *> cache_shader;
	std::map<std::string, Texture *> cache_texture;
	std::map<std::string, RenderTarget *> cache_rendertarget;

	//buffer template
	template <typename T>
	T *GetBuffer(const std::string &name) {
		T *ret = (T *)FindBuffer(name);
		if (!ret) {
			ret = new T;
			ret->SetName(name);
			buffermapper[name] = ret;
		}
		return ret;
	}

	UINT swap_index = 0;
	void *uniform_addr = 0 ;
public:
	struct ConstantFormat {
		float World[16];
		float View[16];
		float Proj[16];
		float Reserved[16];
		float Screen[4];
		float Time[4];
		float Pos[4];
		float At[4];
		float Up[4];
	};

	ConstantFormat *GetConstantBuffer() {
		return (ConstantFormat *)uniform_addr;
	}

	Renderer(HWND hWnd, UINT w, UINT h, UINT framecount) {
		device = new Device(hWnd, w, h, framecount);
		Constant *constant = new Constant(device->Get(), 4096);
		cache_constant["uniform"] = constant;
		uniform_addr = constant->Map();
	}

	~Renderer() {
		DeleteConstant();
		DeleteShader();
		DeleteRenderTarget();
		auto it = buffermapper.begin();
		auto ite = buffermapper.end();
		while (it != ite) {
			if (it->second) {
				printf("%s : %s : Delete buffermapper\n", __FUNCTION__, it->first.c_str());
				delete it->second;
			}
			it++;
		}
		if (device) delete device;
	}

	void DeleteConstant() {
		auto it = cache_constant.begin();
		auto ite = cache_constant.end();
		while (it != ite) {
			printf("%s : %s : Delete cache_constant\n", __FUNCTION__, it->first.c_str());
			delete it->second;
			it++;
		}
	}
	void DeleteTexture() {
		auto it = cache_texture.begin();
		auto ite = cache_texture.end();
		while (it != ite) {
			printf("%s : %s : Delete cache_texture\n", __FUNCTION__, it->first.c_str());
			delete it->second;
			it++;
		}
	}
	void DeleteVertex() {
		auto it = cache_vertex.begin();
		auto ite = cache_vertex.end();
		while (it != ite) {
			printf("%s : %s : Delete cache_vertex\n", __FUNCTION__, it->first.c_str());
			delete it->second;
			it++;
		}
	}

	void DeleteRenderTarget() {
		auto it = cache_rendertarget.begin();
		auto ite = cache_rendertarget.end();
		while (it != ite) {
			printf("%s : %s : Delete cache_rendertarget\n", __FUNCTION__, it->first.c_str());
			delete it->second;
			it++;
		}
	}

	void DeleteShader() {
		auto it = cache_shader.begin();
		auto ite = cache_shader.end();
		while (it != ite) {
			printf("%s : %s : Delete cache_shader\n", __FUNCTION__, it->first.c_str());
			delete it->second;
			it++;
		}
		cache_shader.clear();
	}

	BaseBuffer *FindBuffer(const std::string &name) {
		auto it = buffermapper.find(name);
		if (it != buffermapper.end()) {
			return it->second;
		}
		return 0;
	}

	ViewBuffer *GetViewPort(const std::string &name) {
		return GetBuffer<ViewBuffer>(name);
	}

	VertexBuffer *GetGeometry(const std::string &name) {
		return GetBuffer<VertexBuffer>(name);
	}

	TextureBuffer *GetTexture(const std::string &name) {
		return GetBuffer<TextureBuffer>(name);
	}

	RenderTargetBuffer *GetRenderTarget(const std::string &name) {
		return GetBuffer<RenderTargetBuffer>(name);
	}

	void MarkUpdate(int type) {
		auto it = buffermapper.begin();
		auto ite = buffermapper.end();
		for (; it != ite; it++) {
			auto *buf = it->second;
			if (buf->GetType() == type) {
				buf->MarkUpdate();
			}
		}
	}

	void Update() {
		UINT cmdtype = Device::UPLOAD;
		std::vector<ID3D12Resource *> uploadBufList;
		ID3D12CommandQueue *cmdQueue = device->GetQueue(cmdtype);
		ID3D12CommandAllocator *cmdAlloc = device->GetAllocator(cmdtype);
		ID3D12GraphicsCommandList *cmdList = device->GetList(cmdtype);

		cmdAlloc->Reset();
		cmdList->Reset(cmdAlloc, 0);
		bool need_update = false;
		auto it = buffermapper.begin();
		auto ite = buffermapper.end();
		for (; it != ite; it++) {
			int type = it->second->GetType();
			std::string name = it->second->GetName();
			if (type == BUFFER_TYPE_NONE) {
				printf("%s : %s : type BUFFER_TYPE_NONE\n", __FUNCTION__, name.c_str());
			}

			if (type == BUFFER_TYPE_VERTEX) {
				auto *buf = (VertexBuffer *)it->second;
				if (!buf->IsUpdate()) continue;
				buf->UnmarkUpdate();
				auto *vertex = new Vertex(device->Get(), buf->GetBuffer(), buf->GetBufferSize(), sizeof(VertexBuffer::Format));
				if (vertex->Get()) {
					auto it = cache_vertex.find(name);
					if (it != cache_vertex.end()) {
						delete it->second;
					}
					cache_vertex[name] = vertex;
				} else {
					printf("ERROR : %s cant alloc BUFFER_TYPE_VERTEX resource\n", __FUNCTION__);
				}
			}

			if (type == BUFFER_TYPE_TEXTURE) {
				auto *buf = (TextureBuffer *)it->second;
				if (!buf->IsUpdate()) continue;
				buf->UnmarkUpdate();

				auto *texture = new Texture(device->Get(), buf->GetWidth(), buf->GetHeight());
				if (texture->Get()) {
					if (buf->GetBuffer()) {
						ID3D12Resource *uploadBuf = UploadResource(device->Get(), cmdList, texture->Get(), buf->GetBuffer());
						if (uploadBuf) {
							uploadBufList.push_back(uploadBuf);
							need_update = true;
						}
					} else {
						printf("ERROR %s : cant alloc TEXTURE\n", __FUNCTION__);
					}

					auto it = cache_texture.find(name);
					if (it != cache_texture.end()) {
						delete it->second;
					}
					cache_texture[name] = texture;
				} else {
					printf("ERROR : %s cant alloc BUFFER_TYPE_TEXTURE resource\n", __FUNCTION__);
				}
			}
		}
		cmdList->Close();
		if (need_update) {
			device->Execute(cmdtype);
			device->WaitForCompleteExecute(cmdtype);
			for (auto buf : uploadBufList) {
				RELEASE(buf);
			}
		}

	}

	void Render(const std::string &view_name) {

		using namespace DirectX;
		ViewBuffer *view = (ViewBuffer *)buffermapper[view_name];
		static float time_status = 0;
		time_status += 1.0f / 60.0f;
		if (view) {
			auto *rt = view->GetRenderTarget();
			RenderTarget *curr_rt = NULL;
			UINT rtvCount = 1;
			if (rt) {
				rtvCount = rt->GetCount();
				std::string rt_name = rt->GetName();
				if (cache_rendertarget.find(rt_name) == cache_rendertarget.end()) {
					auto *rendertarget = new RenderTarget(device->Get(), rt->GetWidth(), rt->GetHeight(), rtvCount);
					for (int i = 0 ; i < rtvCount; i++) {
						Texture *texture = rendertarget->GetTexture(i);
						TextureBuffer *color_tex = rt->GetTexture(i, false);
						cache_texture[color_tex->GetName()] = texture;
					}
					{
						Texture *depth = rendertarget->GetDepth();
						TextureBuffer *depth_tex = rt->GetTexture(0, true);
						cache_texture[depth_tex->GetName()] = depth;
					}
					cache_rendertarget[rt_name] = rendertarget;
				}
				curr_rt = cache_rendertarget[rt_name];
			}

			//Viewports
			D3D12_VIEWPORT viewport = CreateViewPort(
			                              view->GetX(), view->GetY(), view->GetW(), view->GetH(), 0.0f, 1.0f);

			//ScissorRects
			D3D12_RECT rect = {};
			rect.right  = view->GetW();
			rect.bottom = view->GetH();

			//ClearColor
			float clear_color[4] = {};
			view->GetClearColor(clear_color);

			//Barrier
			D3D12_RESOURCE_BARRIER BeginBarrier[8] = {};
			D3D12_RESOURCE_BARRIER EndBarrier[8] = {};
			D3D12_CPU_DESCRIPTOR_HANDLE rtvColorHandle[8] = {};
			D3D12_CPU_DESCRIPTOR_HANDLE rtvDepthHandle = {};
			if (curr_rt) {
				for (int i = 0 ; i < rtvCount; i++) {
					rtvColorHandle[i] = CpuDescriptorHandle(device->Get(), curr_rt->GetColorHeap(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i);
					BeginBarrier[i] = ResourceBarrier(curr_rt->GetTexture(i)->Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
					EndBarrier[i] = ResourceBarrier(curr_rt->GetTexture(i)->Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				}
				rtvDepthHandle = CpuDescriptorHandle(device->Get(), curr_rt->GetDepthHeap(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0);
			} else {
				int index = swap_index & 1;
				rtvColorHandle[0] = CpuDescriptorHandle(device->Get(), device->GetBackBufferHeap(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, index);
				BeginBarrier[0] = ResourceBarrier(device->GetBackBuffer(index), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				EndBarrier[0] = ResourceBarrier(device->GetBackBuffer(index), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
				rtvDepthHandle = CpuDescriptorHandle(device->Get(), device->GetDepthBufferHeap(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0);
			}

			//set matrix
			ConstantFormat *cformat = (ConstantFormat *)uniform_addr;
			{
				XMMATRIX ident = XMMatrixIdentity();
				MatrixData view_data = view->GetViewMatrix();
				MatrixData proj_data = view->GetProjMatrix();
				*(XMFLOAT4X4 *)(&cformat->World[0]) = *(XMFLOAT4X4 *)(&ident);
				*(XMFLOAT4X4 *)(&cformat->View[0])  = *(XMFLOAT4X4 *)(&view_data);
				*(XMFLOAT4X4 *)(&cformat->Proj[0])  = *(XMFLOAT4X4 *)(&proj_data);
				cformat->Time[0] = time_status;
				cformat->Screen[0] = view->GetW();
				cformat->Screen[1] = view->GetH();
				cformat->Screen[2] = cformat->Screen[0] / cformat->Screen[1];
				cformat->Screen[3] = 0.0f;
			}

			//apply
			ID3D12CommandQueue *cmdQueue = device->GetQueue();
			ID3D12CommandAllocator *cmdAlloc = device->GetAllocator();
			ID3D12GraphicsCommandList *cmdList = device->GetList();
			cmdAlloc->Reset();
			cmdList->Reset(cmdAlloc, 0);
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &rect);
			cmdList->ResourceBarrier(rtvCount, BeginBarrier);
			cmdList->OMSetRenderTargets(rtvCount, rtvColorHandle, FALSE, &rtvDepthHandle);
			for (int i = 0 ; i < rtvCount; i++) {
				cmdList->ClearRenderTargetView(rtvColorHandle[i], clear_color, 0, NULL);
			}
			cmdList->ClearDepthStencilView(rtvDepthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

			//per vertex
			int count_geo = 0;
			std::vector<VertexBuffer *> temp_vertex_buffer;
			view->GetVertexBuffer(temp_vertex_buffer);
			for (auto vertex : temp_vertex_buffer) {
				std::string shader_name = vertex->GetShaderName();
				if (!shader_name.empty()) {
					auto it = cache_shader.find(shader_name);
					if (it == cache_shader.end()) {
						Shader *shader = new Shader(device->Get(), shader_name.c_str(), curr_rt ? true : false, rtvCount);
						if (shader->GetPipelineState()) {
							auto it = cache_shader.find(shader_name);
							if (it != cache_shader.end()) {
								delete it->second;
							}
							cache_shader[shader_name] = shader;
						} else {
							printf("Failed Create PipelineState\n");
						}
					}
				}

				auto shader_object = cache_shader[shader_name];
				if (!shader_object) {
					continue;
				}
				auto vertex_object = cache_vertex[vertex->GetName()];
				if (!vertex_object) {
					continue;
				}
				if (!shader_object->GetPipelineState()) {
					continue;
				}
				std::vector<ID3D12DescriptorHeap *> vDescHeaps;
				std::map<int, std::string> texbind;
				vertex->GetTextureIndex(texbind);
				for (auto tex : texbind) {
					int loc = tex.first;
					auto texture = cache_texture[tex.second.c_str()];
					if (texture) {
						vDescHeaps.push_back(texture->GetHeap());
					}
				}

				//todo cached.
				cmdList->SetGraphicsRootSignature(shader_object->GetRootSignature());
				cmdList->SetPipelineState(shader_object->GetPipelineState());

				//set heap
				int index = 0;
				{
					Constant *uniform_const = cache_constant["uniform"];
					if (uniform_const) {
						ID3D12DescriptorHeap *uniform_heap = uniform_const->GetHeap();
						cmdList->SetDescriptorHeaps(1, &uniform_heap);
						cmdList->SetGraphicsRootDescriptorTable(index, uniform_const->GetHeap()->GetGPUDescriptorHandleForHeapStart());
					}
					index++;
				}
				if (!vDescHeaps.empty()) {
					for (int i = 0 ; i < vDescHeaps.size(); i++) {
						cmdList->SetDescriptorHeaps(1, &vDescHeaps[i]);
						auto handleSRV = vDescHeaps[i]->GetGPUDescriptorHandleForHeapStart();
						cmdList->SetGraphicsRootDescriptorTable(i + index, handleSRV);
					}
				}
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				D3D12_VERTEX_BUFFER_VIEW view = vertex_object->GetView<D3D12_VERTEX_BUFFER_VIEW>();
				cmdList->IASetVertexBuffers(0, 1, &view);
				cmdList->DrawInstanced(vertex->GetCount(), 1, 0, 0);
				count_geo++;
			}
			temp_vertex_buffer.clear();
			cmdList->ResourceBarrier(rtvCount, EndBarrier);
			cmdList->Close();

			//do.
			device->Execute();

			//wait for GPU
			device->WaitForCompleteExecute();
		}
	}
	void Present() {
		device->SwapBuffer();
		swap_index++;
	}
};
}
