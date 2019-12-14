#pragma once

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "D3DCompiler.lib")

#define tostr(x) #x

#define err(...) printf("[ERR]:" __FILE__ " : " __FUNCTION__ " : " __VA_ARGS__)
#define dbg(...) printf("[DBG]:" __FILE__ " : " __FUNCTION__ " : " __VA_ARGS__)
#define CHECK_DEVICE_STATUS(name)                                      \
	{                                                              \
		auto eee = dev->GetDeviceRemovedReason();              \
		dbg("%s, GetDeviceRemovedReason()=%08X\n", name, eee); \
		if (eee)                                               \
			exit(0);                                       \
	};

namespace dx12util
{

int print_gp_state(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p);
void print_err_hresult(HRESULT hr);

inline D3D12_CPU_DESCRIPTOR_HANDLE
get_handle_cpu(ID3D12Device *dev, ID3D12DescriptorHeap *heap, int idx)
{
	auto desc = heap->GetDesc();
	auto ret = heap->GetCPUDescriptorHandleForHeapStart();

	ret.ptr += dev->GetDescriptorHandleIncrementSize(desc.Type) * idx;
	return (ret);
}

inline D3D12_GPU_DESCRIPTOR_HANDLE
get_handle_gpu(ID3D12Device *dev, ID3D12DescriptorHeap *heap, int idx)
{
	auto desc = heap->GetDesc();
	auto ret = heap->GetGPUDescriptorHandleForHeapStart();

	ret.ptr += dev->GetDescriptorHandleIncrementSize(desc.Type) * idx;
	return (ret);
}


inline UINT
get_res_width(ID3D12Resource *res)
{
	if (!res) {
		err("null");
		return -1;
	}
	auto desc = res->GetDesc();
	return UINT(desc.Width);
}

//https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc
inline UINT
get_res_height(ID3D12Resource *res)
{
	if (!res) {
		err("null");
		return -1;
	}
	auto desc = res->GetDesc();
	return UINT(desc.Height);
}

inline DXGI_FORMAT
get_res_format(ID3D12Resource *res)
{
	if (!res) {
		err("null");
		return DXGI_FORMAT_UNKNOWN;
	}
	auto desc = res->GetDesc();
	return (desc.Format);
}

inline int
create_device(HWND hwnd, ID3D12Device **dev,
	      D3D_FEATURE_LEVEL flevel = D3D_FEATURE_LEVEL_11_1)
{
	D3D12CreateDevice(NULL, flevel, IID_PPV_ARGS(&(*dev)));
	return (0);
}

inline int
create_cmdqueue(ID3D12Device *dev, ID3D12CommandQueue **queue)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};

	dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&(*queue)));
	return (0);
}

inline int
create_cmdallocator(ID3D12Device *dev, ID3D12CommandAllocator **allocator)
{
	auto hr = dev->CreateCommandAllocator(
	    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(*allocator)));

	if (hr)
	{
		err("hr = %08X\n", hr);
		print_err_hresult(hr);
	}
	return (0);
}

inline int
create_cmdlist(
    ID3D12Device *dev, ID3D12CommandAllocator *allocator,
    ID3D12GraphicsCommandList **cmdlist)
{
	auto hr = dev->CreateCommandList(
	    0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr,
	    IID_PPV_ARGS(&(*cmdlist)));

	if (hr)
	{
		err("hr = %08X\n", hr);
		print_err_hresult(hr);
	}
	(*cmdlist)->Close();
	return (0);
}

inline int
create_swap_chain(ID3D12CommandQueue *queue, HWND hwnd,
		  int w, int h, int cnt,
		  IDXGISwapChain3 **swap_chain)
{
	DXGI_SWAP_CHAIN_DESC desc = {};
	IDXGIFactory4 *factory = nullptr;
	IDXGISwapChain *temp = nullptr;

	desc.BufferCount = cnt;
	desc.BufferDesc.Width = w;
	desc.BufferDesc.Height = h;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	desc.Windowed = TRUE;
	desc.OutputWindow = hwnd;

	CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	factory->CreateSwapChain(queue, &desc, &temp);
	temp->QueryInterface(IID_PPV_ARGS(&(*swap_chain)));
	temp->Release();
	factory->Release();
	if (!swap_chain) {
		err("failed\n");
		return (-1);
	}
	return (0);
}

inline int
get_buffer_from_swapchain(
    IDXGISwapChain3 *swap_chain, int idx,
    ID3D12Resource **res)
{
	auto hr = swap_chain->GetBuffer(idx, IID_PPV_ARGS(&(*res)));

	if (hr) {
		err("idx=%08X, hr=%08X\n", idx, hr);
		print_err_hresult(hr);
		return (-1);
	}
	return (0);
}

inline int
create_desc_heap(ID3D12Device *dev, int num,
		 D3D12_DESCRIPTOR_HEAP_TYPE type,
		 ID3D12DescriptorHeap **heap)
{
	HRESULT hr = S_OK;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};

	desc.NumDescriptors = num;
	desc.Type = type;

	switch (type) {
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
	default:
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		break;
	}

	hr = dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&(*heap)));
	if (hr) {
		err("type=%08X, hr=%08X\n", type, hr);
		print_err_hresult(hr);
		return (hr);
	}
	return (0);
}

inline int
get_miplevels(int w, int h)
{
	int min_value = (std::min)(w, h);
	int level = 0;

	while (min_value) {
		level++;
		min_value >>= 1;
	}
	return level;
}

inline int
create_res(ID3D12Device *dev, int w, int h, DXGI_FORMAT fmt,
	   D3D12_RESOURCE_FLAGS flags, BOOL is_upload, ID3D12Resource **res)
{
	HRESULT hr = S_OK;
	D3D12_RESOURCE_DESC desc = {};
	D3D12_HEAP_PROPERTIES heap_prop = {};
	int miplevels = get_miplevels(w, h);

	heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_prop.CreationNodeMask = 1;
	heap_prop.VisibleNodeMask = 1;

	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = w;
	desc.Height = h;
	desc.Format = fmt;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = miplevels;

	if (is_upload)
	{
		heap_prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.MipLevels = 1;
	}

	hr = dev->CreateCommittedResource(
	    &heap_prop, D3D12_HEAP_FLAG_NONE, &desc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr, IID_PPV_ARGS(&(*res)));
	if (hr)
	{
		err("w=%d, h=%d, flags=%08X, hr=%08X\n",
		    w, h, flags, hr);
		print_err_hresult(hr);
		err("GetDeviceRemovedReason=%08X\n",
		    dev->GetDeviceRemovedReason());
		return (hr);
	}
	return (0);
}


inline int
create_rtv(ID3D12Device *dev, ID3D12Resource *res,
	   D3D12_CPU_DESCRIPTOR_HANDLE hcpu_rtv, int mlevel = 0)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc_rtv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_rtv.Texture2D.MipSlice = mlevel;
	desc_rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc_rtv.Format = desc_res.Format;
	dev->CreateRenderTargetView(res, &desc_rtv, hcpu_rtv);
	return (0);
}

inline int
create_srv(ID3D12Device *dev, ID3D12Resource *res,
	   D3D12_CPU_DESCRIPTOR_HANDLE hcpu_srv, int mlevel)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc_srv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_srv.Format = desc_res.Format;
	desc_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc_srv.Shader4ComponentMapping =
	    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc_srv.Texture2D.MipLevels = mlevel;
	dev->CreateShaderResourceView(res, &desc_srv, hcpu_srv);
	return (0);
}

inline int
create_dsv(ID3D12Device *dev, ID3D12Resource *res,
	   D3D12_CPU_DESCRIPTOR_HANDLE hcpu_dsv)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc_dsv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc_dsv.Format = desc_res.Format;
	dev->CreateDepthStencilView(res, &desc_dsv, hcpu_dsv);
	return (0);
}

inline int
create_cbv(ID3D12Device *dev, ID3D12Resource *res,
	   D3D12_CPU_DESCRIPTOR_HANDLE hcpu_cbv)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc_cbv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_cbv.SizeInBytes = desc_res.Width;
	desc_cbv.BufferLocation = res->GetGPUVirtualAddress();
	dev->CreateConstantBufferView(&desc_cbv, hcpu_cbv);
	return (0);
}

inline int
create_uav(ID3D12Device *dev, ID3D12Resource *res,
	   D3D12_CPU_DESCRIPTOR_HANDLE hcpu_srv)
{
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc_uav = {};

	desc_uav.Format = desc_res.Format;
	desc_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc_uav.Texture2D.MipSlice = 0;
	desc_uav.Texture2D.PlaneSlice = 0;
	dev->CreateUnorderedAccessView(res, nullptr, &desc_uav, hcpu_srv);
	return (0);
}

inline int
create_fence(ID3D12Device *dev, ID3D12Fence **fence)
{
	dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&(*fence)));
	return (0);
}

inline int
create_sampler(ID3D12Device *dev,
	       D3D12_FILTER filter, D3D12_CPU_DESCRIPTOR_HANDLE hcpu)
{
	D3D12_SAMPLER_DESC desc = {};

	desc.Filter = filter;
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.MinLOD = 0;
	desc.MaxLOD = D3D12_FLOAT32_MAX;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 1;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	dev->CreateSampler(&desc, hcpu);
	return (0);
}

inline int
create_gp_state(ID3D12Device *dev,
		D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc,
		ID3D12RootSignature *root_sig,
		ID3D12PipelineState **state)
{
	HRESULT hr = S_OK;

	desc->pRootSignature = root_sig;
	print_gp_state(desc);
	hr = dev->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&(*state)));
	if (hr)
	{
		err("Failed hr=%08X\n", hr);
		print_err_hresult(hr);
		return (-1);
	}
	else
	{
		dbg("Success hr=%08X\n", hr);
	}
	return (0);
}

inline int
set_gp_rs_reset(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p)
{
	auto &rsstate = p->RasterizerState;

	rsstate.FillMode = D3D12_FILL_MODE_SOLID;
	rsstate.CullMode = D3D12_CULL_MODE_BACK;
	rsstate.DepthClipEnable = TRUE;
	rsstate.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rsstate.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rsstate.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rsstate.FrontCounterClockwise = FALSE;
	rsstate.MultisampleEnable = FALSE;
	rsstate.AntialiasedLineEnable = FALSE;
	rsstate.ForcedSampleCount = 0;
	rsstate.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return (0);
}

inline int
set_gp_rs_fillmode(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p, BOOL isSolid)
{
	auto &rsstate = p->RasterizerState;

	rsstate.FillMode = isSolid ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
	return (0);
}

inline int
set_gp_dss(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
	   BOOL depth_enable, BOOL stencil_enable)
{
	auto &dsstate = p->DepthStencilState;

	dsstate.DepthEnable = depth_enable;
	dsstate.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	dsstate.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsstate.StencilEnable = stencil_enable;
	dsstate.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsstate.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	dsstate.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	dsstate.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	dsstate.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	dsstate.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	dsstate.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	dsstate.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	dsstate.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	dsstate.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	return (0);
}

inline int
set_gp_rtv_format(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		  int idx, DXGI_FORMAT fmt)
{
	p->RTVFormats[idx] = fmt;
	return (0);
}

inline int
set_gp_dsv_format(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		  DXGI_FORMAT fmt)
{
	p->DSVFormat = fmt;
	return (0);
}

inline int
set_gp_rt_blend_state(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		      int idx, BOOL blend_enable)
{
	auto &bstate = p->BlendState.RenderTarget[idx];

	bstate.BlendEnable = blend_enable;
	bstate.LogicOpEnable = FALSE;
	bstate.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	bstate.DestBlend = D3D12_BLEND_INV_DEST_ALPHA;
	bstate.BlendOp = D3D12_BLEND_OP_ADD;
	bstate.SrcBlendAlpha = D3D12_BLEND_ONE;
	bstate.DestBlendAlpha = D3D12_BLEND_ZERO;
	bstate.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	bstate.LogicOp = D3D12_LOGIC_OP_NOOP;
	bstate.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	return (0);
}

inline int
set_gp_independent_blend(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
			 BOOL value)
{
	p->BlendState.IndependentBlendEnable = value;
	return (0);
}

inline int
set_gp_prim_type(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		 D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	p->PrimitiveTopologyType = type;
	return (0);
}

inline int
set_gp_vs_bytecode(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		   const void *bytecode, size_t size)
{
	p->VS.pShaderBytecode = bytecode;
	p->VS.BytecodeLength = size;
	return (0);
}

inline int
set_gp_ps_bytecode(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		   const void *bytecode, size_t size)
{
	p->PS.pShaderBytecode = bytecode;
	p->PS.BytecodeLength = size;
	return (0);
}

inline int
set_input_elements(D3D12_INPUT_ELEMENT_DESC *pdescs, int idx,
		   LPCSTR name, UINT num_component, UINT slot, BOOL per_instance)
{
	auto &desc = pdescs[idx];
	UINT offset = 0;

	desc.InputSlot = slot;
	desc.SemanticName = name;
	desc.SemanticIndex = 0;
	switch (num_component)
	{
	case 1:
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	case 2:
		desc.Format = DXGI_FORMAT_R32G32_FLOAT;
		break;
	case 3:
		desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case 4:
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	default:
		err("Invalid num_component=%d\n", num_component);
		return (-1);
	}
	for (int i = 0; i < idx; i++)
	{
		auto &d = pdescs[i];

		switch (d.Format)
		{
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_FLOAT:
			offset += sizeof(float) * 1;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
			offset += sizeof(float) * 2;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
			offset += sizeof(float) * 3;
			break;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
			offset += sizeof(float) * 4;
			break;
		default:
			err("Unknwon idx=%d, d.Format=%d\n", idx, d.Format);
			return (-1);
		}
	}
	desc.AlignedByteOffset = offset;
	desc.InputSlotClass = (per_instance == TRUE) ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	desc.InstanceDataStepRate = (per_instance == TRUE) ? 1 : 0;
	return (0);
}

inline int
set_gp_input_element(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p,
		     D3D12_INPUT_ELEMENT_DESC *pdesc, int num)
{
	p->InputLayout.pInputElementDescs = pdesc;
	p->InputLayout.NumElements = num;
	return (0);
}

inline int
init_gp_state(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p)
{
	p->SampleMask = UINT_MAX;
	p->NodeMask = 0x0;
	p->NumRenderTargets = _countof(p->BlendState.RenderTarget);
	p->SampleDesc.Count = 1;
	p->BlendState.AlphaToCoverageEnable = FALSE;
	set_gp_input_element(p, nullptr, 0);
	set_gp_rs_fillmode(p, TRUE);
	set_gp_rs_reset(p);
	set_gp_dss(p, FALSE, FALSE);
	set_gp_prim_type(p, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	set_gp_dsv_format(p, DXGI_FORMAT_D32_FLOAT);
	set_gp_independent_blend(p, FALSE);
	for (int i = 0; i < _countof(p->BlendState.RenderTarget); i++)
	{
		set_gp_rt_blend_state(p, i, FALSE);
		set_gp_rtv_format(p, i, DXGI_FORMAT_R8G8B8A8_UNORM);
	}
	return (0);
}

inline int
set_desc_range(D3D12_DESCRIPTOR_RANGE *p,
	       D3D12_DESCRIPTOR_RANGE_TYPE type,
	       UINT num_desc,
	       UINT base_reg)
{
	p->RangeType = type;
	p->NumDescriptors = num_desc;
	p->BaseShaderRegister = base_reg;
	p->OffsetInDescriptorsFromTableStart =
	    D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	return (0);
}

inline int
set_root_param(
    D3D12_ROOT_PARAMETER *p,
    D3D12_DESCRIPTOR_RANGE *ranges,
    UINT num, D3D12_ROOT_PARAMETER_TYPE type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
{
	p->ParameterType = type;
	p->DescriptorTable.pDescriptorRanges = ranges;
	p->DescriptorTable.NumDescriptorRanges = num;
	p->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return (0);
}

inline int
set_static_sampler(D3D12_STATIC_SAMPLER_DESC *p, D3D12_FILTER filter, UINT idx)
{
	p->Filter = filter;
	p->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	p->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	p->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	p->MipLODBias = 0;
	p->MaxAnisotropy = 0;
	p->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	p->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	p->MinLOD = 0.0f;
	p->MaxLOD = D3D12_FLOAT32_MAX;
	p->ShaderRegister = idx;
	p->RegisterSpace = 0;
	p->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	return (0);
}

inline int
create_root_sig(ID3D12Device *dev, ID3D12RootSignature **root_sig,
		int num_srv,
		int num_cbv,
		int num_uav,
		int num_sampler)
{
	HRESULT hr = S_OK;
	ID3DBlob *pblob = nullptr;
	ID3DBlob *perrblob = nullptr;

	D3D12_ROOT_CONSTANTS root_constants = {};
	std::vector<D3D12_ROOT_PARAMETER> root_params;
	std::vector<D3D12_DESCRIPTOR_RANGE> desc_ranges;
	D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
	D3D12_STATIC_SAMPLER_DESC sampler[2] = {};

	set_static_sampler(&sampler[0], D3D12_FILTER_MIN_MAG_MIP_POINT, 0);
	set_static_sampler(&sampler[1], D3D12_FILTER_MIN_MAG_MIP_LINEAR, 1);

	desc_ranges.resize(num_srv + num_cbv + num_uav + num_sampler);
	int range_index = 0;

	{
		D3D12_ROOT_PARAMETER roots = {};
		memset(&roots, 0, sizeof(roots));
		roots.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		roots.Constants = {0, 0, 32};
		root_params.push_back(roots);
	}
	for (UINT i = 0; i < num_srv; i++)
	{
		D3D12_DESCRIPTOR_RANGE *range = &desc_ranges[range_index];
		D3D12_ROOT_PARAMETER roots = {};
		roots.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		set_desc_range(range, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i);
		set_root_param(&roots, range, 1);
		root_params.push_back(roots);
		range_index++;
	}
	for (UINT i = 0; i < num_uav; i++)
	{
		D3D12_DESCRIPTOR_RANGE *range = &desc_ranges[range_index];
		D3D12_ROOT_PARAMETER roots = {};
		roots.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		set_desc_range(range, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, i);
		set_root_param(&roots, range, 1);
		root_params.push_back(roots);
		range_index++;
	}

	printf("root_params.data()=%p, root_params.size()=%d\n",
	       root_params.data(), root_params.size());
	root_sig_desc.pParameters = root_params.data();
	root_sig_desc.NumParameters = root_params.size();
	root_sig_desc.pStaticSamplers = sampler;
	root_sig_desc.NumStaticSamplers = _countof(sampler);
	root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	hr = D3D12SerializeRootSignature(
	    &root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
	    &pblob, &perrblob);
	if (hr && perrblob)
	{
		err("D3D12SerializeRootSignature:\n%s\n",
		    (char *)perrblob->GetBufferPointer());
		return (-1);
	}
	hr = dev->CreateRootSignature(
	    0, pblob->GetBufferPointer(), pblob->GetBufferSize(),
	    IID_PPV_ARGS(&(*root_sig)));
	if (perrblob)
		perrblob->Release();
	if (pblob)
		pblob->Release();

	if (hr) {
		err("Failed hr=%08X\n", hr);
		return (-1);
	}
	return (0);
}

inline int
upload_data(ID3D12Resource *res, void *data, size_t size)
{
	UINT8 *dest = nullptr;

	res->Map(0, NULL, reinterpret_cast<void **>(&dest));
	if (!dest) {
		err("Failed map\n");
		return (-1);
	}
	memcpy(dest, data, size);
	res->Unmap(0, NULL);
	return (0);
}

inline int
compile_shader_from_file(
    const char *fname,
    const char *entry,
    const char *profile,
    std::vector<uint8_t> &shader_code)
{
	ID3DBlob *blob = nullptr;
	ID3DBlob *blob_err = nullptr;
	ID3DBlob *blob_sig = nullptr;

	std::string fstr = fname;
	std::vector<WCHAR> wfname;
	UINT flags = 0;

	if (!fname) {
		err("Invalid Filename\n", __FUNCTION__);
		return (-1);
	}

	for (int i = 0; i < fstr.length(); i++)
		wfname.push_back(fstr[i]);
	wfname.push_back(0);
	D3DCompileFromFile(
	    &wfname[0], NULL,
	    D3D_COMPILE_STANDARD_FILE_INCLUDE,
	    entry, profile, flags, 0, &blob, &blob_err);

	if (blob_err)
		err("Message:\n%s\n", (char *)blob_err->GetBufferPointer());

	if (!blob) {
		if (blob_err)
			blob_err->Release();

		if (!blob_err)
			err("File Not found : %s\n", fname);
		return (-1);
	}
	shader_code.resize(blob->GetBufferSize());
	memcpy(shader_code.data(),
	       blob->GetBufferPointer(),
	       blob->GetBufferSize());
	if (blob_err)
		blob->Release();
	if (blob)
		blob->Release();
	return (0);
}

inline int
print_gp_state(D3D12_GRAPHICS_PIPELINE_STATE_DESC *p)
{
	dbg("p->pRootSignature=0x%016llX\n",
	    (uint64_t)p->pRootSignature);
	dbg("p->VS.pShaderBytecode=0x%016llX\n",
	    (uint64_t)p->VS.pShaderBytecode);
	dbg("p->VS.BytecodeLength=0x%016llX\n",
	    (uint64_t)p->VS.BytecodeLength);
	dbg("p->PS.pShaderBytecode=0x%016llX\n",
	    (uint64_t)p->PS.pShaderBytecode);
	dbg("p->PS.BytecodeLength=0x%016llX\n",
	    (uint64_t)p->PS.BytecodeLength);
	dbg("p->SampleDesc.Count=0x%016llX\n",
	    (uint64_t)p->SampleDesc.Count);
	dbg("p->BlendState.AlphaToCoverageEnable=0x%016llX\n",
	    (uint64_t)p->BlendState.AlphaToCoverageEnable);
	dbg("p->SampleMask=0x%016llX\n",
	    (uint64_t)p->SampleMask);
	dbg("p->NodeMask=0x%016llX\n",
	    (uint64_t)p->NodeMask);
	dbg("p->NumRenderTargets=0x%016llX\n",
	    (uint64_t)p->NumRenderTargets);
	dbg("p->RasterizerState.FillMode=0x%016llX\n",
	    (uint64_t)p->RasterizerState.FillMode);
	dbg("p->InputLayout.pInputElementDescs=0x%016llX\n",
	    (uint64_t)p->InputLayout.pInputElementDescs);
	dbg("p->InputLayout.NumElements=0x%016llX\n",
	    (uint64_t)p->InputLayout.NumElements);
	for (int i = 0; i < p->InputLayout.NumElements; i++) {
		auto &iref = p->InputLayout.pInputElementDescs[i];
		dbg("p->InputLayout.pInputElementDescs[%d].name=%s\n",
		    i, iref.SemanticName);
	}
	return (0);
}

void print_err_hresult(HRESULT hr)
{
	switch (hr)
	{
	case 0x00000000:
		err("S_OK            \n");
		break;
	case 0x80004004:
		err("E_ABORT         \n");
		break;
	case 0x80070005:
		err("E_ACCESSDENIED  \n");
		break;
	case 0x80004005:
		err("E_FAIL          \n");
		break;
	case 0x80070006:
		err("E_HANDLE        \n");
		break;
	case 0x80070057:
		err("E_INVALIDARG    \n");
		break;
	case 0x80004002:
		err("E_NOINTERFACE   \n");
		break;
	case 0x80004001:
		err("E_NOTIMPL       \n");
		break;
	case 0x8007000E:
		err("E_OUTOFMEMORY   \n");
		break;
	case 0x80004003:
		err("E_POINTER       \n");
		break;
	case 0x8000FFFF:
		err("E_UNEXPECTED    \n");
		break;
	default:
		err("UNKNOWN\n");
		break;
	}
}

} // namespace dx12util
