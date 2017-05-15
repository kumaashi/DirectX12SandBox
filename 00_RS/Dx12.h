#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <map>
#include <pix.h>

#ifndef RELEASE
#define RELEASE(x) {if(x) {x->Release(); x = NULL; } }
#endif

enum {
	BUFFER_TYPE_NONE = 0,
	BUFFER_TYPE_SHADER,
	BUFFER_TYPE_VERTEX,
	BUFFER_TYPE_TEXTURE,
	BUFFER_TYPE_RENDERTARGET,
	BUFFER_TYPE_INSTANCING,
	BUFFER_TYPE_VIEW,
};

struct VectorData {
	float data[4];
};

struct MatrixData {
	float data[16];
	void Print() {
		for (int i = 0; i < 16; i++) {
			if ( (i % 4) == 0) printf("\n");
			printf("%f, ", data[i]);
		}
	}
};

class BaseBuffer {
	std::string name;
	int type;
	bool is_update;
public:
	BaseBuffer() {
		name = "";
		type = BUFFER_TYPE_NONE;
		is_update = false;
	}

	virtual ~BaseBuffer() {}

	void SetType(int t) {
		type = t;
	}

	int GetType() {
		return type;
	}

	std::string GetName() {
		return name;
	}

	void SetName(const std::string &s) {
		name = s;
	}

	void MarkUpdate() {
		is_update = true;
	}

	void UnmarkUpdate() {
		is_update = false;
	}

	bool IsUpdate() {
		return is_update;
	}
};


class VertexBuffer;
struct InstancingBuffer : public BaseBuffer {
public:
	struct Data {
		MatrixData world;
		VectorData color;
	};
	InstancingBuffer() {
		SetType(BUFFER_TYPE_INSTANCING);
		vdata.clear();
	}
	virtual ~InstancingBuffer() {
	}
	void ClearData() {
		vdata.clear();
	}
	void AddData(const Data &data) {
		vdata.push_back(data);
	}
	void GetData(std::vector<Data> &dest) {
		dest = vdata;
	}
	void SetVertexBuffer(std::string &name, VertexBuffer *a) {
		vmap[name] = a;
	}

	void DelVertexBuffer(std::string &a) {
		vmap.erase(a);
	}

	void GetVertexBuffer(std::vector<VertexBuffer *> &dest) {
		auto it = vmap.begin();
		auto ite = vmap.end();
		while (it != ite) {
			dest.push_back(it->second);
			it++;
		}
	}

private:
	//positionition and color list
	std::vector<Data> vdata;

	//draw geometry lists
	std::map<std::string, VertexBuffer *> vmap;
};

class TextureBuffer : public BaseBuffer {
	int width;
	int height;
	int format;
	std::vector<char> buffer;
public:
	TextureBuffer() {
		SetType(BUFFER_TYPE_TEXTURE);
		width = 0;
		height = 0;
		format = 0;
	}

	virtual ~TextureBuffer() {
	}

	void Create(int w, int h, int fmt) {
		width = w;
		height = h;
		format = fmt;
	}

	void SetData(void *data, size_t size) {
		buffer.resize(size);
		memcpy(&buffer[0], data, size);
		MarkUpdate();
	}

	bool GetData(std::vector<char> &dest) {
		if (!buffer.empty()) {
			std::copy(buffer.begin(), buffer.end(), std::back_inserter(dest) );
			return true;
		}
		return false;
	}

	void *GetBuffer() {
		if (!buffer.empty()) {
			return &buffer[0];
		}
		return 0;
	}

	int GetWidth() {
		return width;
	}
	int GetHeight() {
		return height;
	}
	int GetFormat() {
		return format;
	}
};

class RenderTargetBuffer : public BaseBuffer {
	enum {
		MAX = 4,
	};
	TextureBuffer color_texture[MAX];
	TextureBuffer depth_texture;
	int width;
	int height;
	int format;
	int type;
	int count;
	float clear_color[4];
public:
	enum {
		TypeBackBuffer = 0,
		TypeTargetBuffer,
		TypeMax,
	};
	RenderTargetBuffer() {
		SetType(BUFFER_TYPE_RENDERTARGET);
		width = 0;
		height = 0;
		format = 0;
		type = 0;
		clear_color[0] = 1.0f;
		clear_color[1] = 0.0f;
		clear_color[2] = 1.0f;
		clear_color[3] = 1.0f;
	}

	virtual ~RenderTargetBuffer() {
	}

	void Create(int w, int h, int fmt, int t, int cnt) {
		width = w;
		height = h;
		format = fmt;
		type = t;
		count = cnt;
		for (int i = 0 ; i < MAX; i++) {
			char name[0x120];
			sprintf(name, "_%d", i);
			color_texture[i].SetName(GetName() + std::string(name));
			color_texture[i].Create(w, h, fmt);
		}
		{
			depth_texture.SetName(GetName() + std::string("_depth"));
			depth_texture.Create(w, h, fmt);
		}
	}
	int GetWidth() {
		return width;
	}
	int GetHeight() {
		return height;
	}
	int GetFormat() {
		return format;
	}
	int GetType() {
		return type;
	}

	TextureBuffer *GetTexture(int i, bool is_depth = false) {
		if (is_depth) {
			return &depth_texture;
		}
		return &color_texture[i];
	}

	const int GetCount() {
		return count;
	}

	void SetClearColor(float r, float g, float b, float a) {
		clear_color[0] = r;
		clear_color[1] = g;
		clear_color[2] = b;
		clear_color[3] = a;
	}

	void GetClearColor(float *a) {
		for (int i = 0; i < 4; i++) {
			a[i] = clear_color[i];
		}
	}
};


class ViewBuffer : public BaseBuffer {
	MatrixData view;
	MatrixData proj;
	MatrixData prev_view;
	MatrixData prev_proj;

	RenderTargetBuffer *rendertarget;
	float vp_x;
	float vp_y;
	float vp_w;
	float vp_h;
	float clearcolor[4];
	float effectparam[4];
	int order;
	std::map<std::string, InstancingBuffer *> vimap;
public:
	ViewBuffer() {
		SetType(BUFFER_TYPE_VIEW);
		rendertarget = 0;
		vp_x = 0;
		vp_y = 0;
		vp_w = 0;
		vp_h = 0;
		order = 0;
	}

	virtual ~ViewBuffer() {
		rendertarget = 0;
	}

	void SetInstancingBuffer(std::string &name, InstancingBuffer *a) {
		vimap[name] = a;
	}

	void DelInstancingBuffer(std::string &a) {
		vimap.erase(a);
	}

	void GetInstancingBuffer(std::vector<InstancingBuffer *> &dest) {
		for (auto & it : vimap) {
			dest.push_back(it.second);
		}
	}

	void GetClearColor(float a[4]) {
		for (int i = 0 ; i < 4; i++) {
			a[i] = clearcolor[i];
		}
	}

	void SetClearColor(float r, float g, float b, float a) {
		clearcolor[0] = r;
		clearcolor[1] = g;
		clearcolor[2] = b;
		clearcolor[3] = a;
	}


	void GetEffectParam(float a[4]) {
		for (int i = 0 ; i < 4; i++) {
			a[i] = effectparam[i];
		}
	}
	void SetEffectParam(float x, float y, float z, float w) {
		effectparam[0] = x;
		effectparam[1] = y;
		effectparam[2] = z;
		effectparam[3] = w;
	}

	void SetOrder(int n) {
		order = n;
	}

	int GetOrder() {
		return order;
	}

	MatrixData GetViewMatrix() {
		return view;
	}

	MatrixData GetProjMatrix() {
		return proj;
	}

	MatrixData GetPrevViewMatrix() {
		return prev_view;
	}

	MatrixData GetPrevProjMatrix() {
		return prev_proj;
	}

	void SetViewMatrix(MatrixData &m) {
		prev_view = view;
		view = m;
	}

	void SetProjMatrix(MatrixData &m) {
		prev_proj = proj;
		proj = m;
	}

	float GetX() {
		return vp_x;
	}

	float GetY() {
		return vp_y;
	}

	float GetW() {
		return vp_w;
	}

	float GetH() {
		return vp_h;
	}

	void SetViewPort(float x, float y, float w, float h) {
		vp_x = x;
		vp_y = y;
		vp_w = w;
		vp_h = h;
	}

	void SetRenderTarget(RenderTargetBuffer *p) {
		rendertarget = p;
	}

	RenderTargetBuffer *GetRenderTarget() {
		return rendertarget;
	}
};

class VertexBuffer : public BaseBuffer {
public:
	typedef enum {
		TRIANGLES = 0,
		PATCHES,
	} Topologie;
	enum {
		Vertex = 0,
		Normal,
		Uv,
		Color,
		Max,
	};
	//mendo
	struct Format {
		float position [3] ;
		float normal [3] ;
		float uv_position [2] ;
		float color[4] ;
	};
	VertexBuffer() {
		SetType(BUFFER_TYPE_VERTEX);
		topologie = TRIANGLES;
	}

	virtual ~VertexBuffer() {
	}

	void SetMatrix(const std::string &name, const MatrixData &e) {
		uniform_mat4[name] = e;
	}

	void GetMatrix(std::map<std::string, MatrixData> &m) {
		auto it = uniform_mat4.begin();
		auto ite = uniform_mat4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

	void SetVector(const std::string &name, const VectorData &e) {
		uniform_vec4[name] = e;
	}

	void GetVector(std::map<std::string, VectorData> &m) {
		auto it = uniform_vec4.begin();
		auto ite = uniform_vec4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}
	void Create(int size) {
		buffer.resize(size);
		memset(&buffer[0], 0, sizeof(Format) * size);
	}

	void SetPos(int index, float x, float y, float z) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].position[0] = x;
		buffer[index].position[1] = y;
		buffer[index].position[2] = z;
	}
	void SetNormal(int index, float x, float y, float z) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].normal[0] = x;
		buffer[index].normal[1] = y;
		buffer[index].normal[2] = z;
	}

	void SetUv(int index, float u, float v) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].uv_position[0] = u;
		buffer[index].uv_position[1] = v;
	}

	void SetColor(int index, float x, float y, float z, float w) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].color[0] = x;
		buffer[index].color[1] = y;
		buffer[index].color[2] = z;
		buffer[index].color[3] = w;
	}

	VertexBuffer::Format *GetBuffer() {
		return &buffer[0];
	}

	size_t GetBufferSize() {
		return buffer.size() * sizeof(VertexBuffer::Format);
	}

	size_t GetCount() {
		return buffer.size();
	}

	void SetShaderName(const std::string &name) {
		shader_name0 = name;
	}

	std::string GetShaderName() {
		return shader_name0;
	}

	Topologie GetTopologie() {
		return topologie;
	}

	void SetTopologie(Topologie t) {
		topologie = t;
	}

	void SetTextureName(const std::string &texname, TextureBuffer *tex) {
		if (tex) {
			uniform_texture_name[texname] = tex->GetName();
		}
	}

	void SetTextureIndex(int idx, TextureBuffer *tex) {
		if (tex) {
			uniform_texture_index[idx] = tex->GetName();
		}
	}

	void GetTextureName(std::map<std::string, std::string> &m) {
		auto it = uniform_texture_name.begin();
		auto ite = uniform_texture_name.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}
	void GetTextureIndex(std::map<int, std::string> &m) {
		auto it = uniform_texture_index.begin();
		auto ite = uniform_texture_index.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

private:
	Topologie topologie;
	std::vector<Format> buffer;
	std::map<std::string, MatrixData> uniform_mat4;
	std::map<std::string, VectorData> uniform_vec4;
	std::map<std::string, std::string> uniform_texture_name;
	std::map<int, std::string> uniform_texture_index;
	std::string shader_name0;
};

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
		{ "POSITION",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV",            0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT,           1,  0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	};

	D3D12_INPUT_LAYOUT_DESC desc = {};
	desc.pInputElementDescs      = inputElementDescs;
	desc.NumElements             = _countof(inputElementDescs);
	return desc;
}


inline ID3D12Device *CreateDevice() {
	ID3D12Device *ret = NULL;
	D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&ret));
	//D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ret));
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

	D3D12_RESOURCE_DESC resource_desc = CreateResourceDesc(dimension, width, height, format, flag);
	device->CreateCommittedResource(
	    &heap_prop,
	    D3D12_HEAP_FLAG_NONE,
	    &resource_desc,
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
		printf("*******************************************************\n");
		printf("fail texture update!!!!\n");
		printf("*******************************************************\n");
		return NULL;
	}
	ID3D12Resource *uploadBuf = CreateCommittedResource(
	                                device,
	                                uploadSize);
	if (!uploadBuf) {
		printf("*******************************************************\n");
		printf("fail texture update : uploadBuf cant alloc!\n");
		printf("*******************************************************\n");
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
		printf("%s : Create : size=%d, SizeInBytes=%d\n", __FUNCTION__, size, SizeInBytes);
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
				printf("%s : Warning : cant create root signature from hlsl\n", __FUNCTION__);
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

	UINT uniform_page_index = 0;
	void *uniform_addr[2] = {};
public:
	enum {
		MAX_INSTANCING_NUMBER = 128,
	};
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
		float Effect[4];
		float Pad1[4];
		float Pad2[4];

		//Dx12::Constant::Constant : Create : size=49536, SizeInBytes=49664
		InstancingBuffer::Data instData[MAX_INSTANCING_NUMBER];
	};

	//ページ
	ConstantFormat *GetConstantBuffer(int page = 0) {
		return (ConstantFormat *)uniform_addr[page];
	}

	Renderer(HWND hWnd, UINT w, UINT h, UINT framecount) {
		device = new Device(hWnd, w, h, framecount);
		Constant *constant0 = new Constant(device->Get(), sizeof(ConstantFormat));
		Constant *constant1 = new Constant(device->Get(), sizeof(ConstantFormat));
		cache_constant["uniform0"] = constant0;
		cache_constant["uniform1"] = constant1;
		uniform_addr[0] = constant0->Map();
		uniform_addr[1] = constant1->Map();
	}

	~Renderer() {
		DeleteConstant();
		DeleteShader();
		DeleteRenderTarget();
		for (auto it : buffermapper) {
			if (it.second) {
				printf("%s : %s : Delete buffermapper\n", __FUNCTION__, it.first.c_str());
				delete it.second;
			}
		}
		if (device) delete device;
	}

	void DeleteConstant() {
		for (auto it : cache_constant) {
			printf("%s : %s : Delete cache_constant\n", __FUNCTION__, it.first.c_str());
			delete it.second;
		}
		cache_constant.clear();
	}
	void DeleteTexture() {
		for (auto it : cache_texture) {
			printf("%s : %s : Delete cache_texture\n", __FUNCTION__, it.first.c_str());
			delete it.second;
		}
		cache_texture.clear();
	}
	void DeleteVertex() {
		for (auto it : cache_vertex) {
			printf("%s : %s : Delete cache_vertex\n", __FUNCTION__, it.first.c_str());
			delete it.second;
		}
		cache_vertex.clear();
	}

	void DeleteRenderTarget() {
		for (auto it : cache_rendertarget) {
			printf("%s : %s : Delete cache_rendertarget\n", __FUNCTION__, it.first.c_str());
			delete it.second;
		}
		cache_rendertarget.clear();
	}

	void DeleteShader() {
		for (auto it : cache_shader) {
			printf("%s : %s : Delete cache_shader\n", __FUNCTION__, it.first.c_str());
			delete it.second;
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

	InstancingBuffer *GetInstancing(const std::string &name) {
		return GetBuffer<InstancingBuffer>(name);
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
			ConstantFormat *cformat = (ConstantFormat *)GetConstantBuffer(0);
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


				//effect param
				float effectparam[4];
				view->GetEffectParam(effectparam);
				cformat->Effect[0] = effectparam[0];
				cformat->Effect[1] = effectparam[1];
				cformat->Effect[2] = effectparam[2];
				cformat->Effect[3] = effectparam[3];
			}

			//apply
			//per view instancing
			std::vector<InstancingBuffer *> temp_instancing_buffer;
			view->GetInstancingBuffer(temp_instancing_buffer);

			//clear
			ID3D12CommandQueue *cmdQueue = device->GetQueue();
			ID3D12CommandAllocator *cmdAlloc = device->GetAllocator();
			ID3D12GraphicsCommandList *cmdList = device->GetList();
			{
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
				cmdList->ResourceBarrier(rtvCount, EndBarrier);
				cmdList->Close();
				device->Execute();
				device->WaitForCompleteExecute();
			}

			//per instancing bundle.
			for (auto & instancing : temp_instancing_buffer) {
				//fill instancing buffer data
				std::vector<InstancingBuffer::Data> instancing_data;
				instancing->GetData(instancing_data);
				const size_t instance_count = instancing_data.size();
				size_t inst_count = 0;
				size_t emit_count = 0;

				//per instance
				while(inst_count < instance_count) {
					emit_count = 0;
					//per instance data and increment inst_count
					for(int i = 0 ; i < MAX_INSTANCING_NUMBER; i++) {
						cformat->instData[i] = instancing_data[inst_count];
						inst_count++;
						emit_count++;
						if(inst_count >= instance_count) {
							break;
						}
					}

					cmdAlloc->Reset();
					cmdList->Reset(cmdAlloc, 0);
					cmdList->RSSetViewports(1, &viewport);
					cmdList->RSSetScissorRects(1, &rect);
					cmdList->ResourceBarrier(rtvCount, BeginBarrier);
					cmdList->OMSetRenderTargets(rtvCount, rtvColorHandle, FALSE, &rtvDepthHandle);

					//per vertex
					int count_geo = 0;
					std::vector<VertexBuffer *> temp_vertex_buffer;
					instancing->GetVertexBuffer(temp_vertex_buffer);
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
							Constant *uniform_const = 0;
							GetConstantBuffer(uniform_page_index);
							if(uniform_page_index == 0) {
								uniform_const = cache_constant["uniform0"];
							} else {
								uniform_const = cache_constant["uniform1"];
							}

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
						cmdList->DrawInstanced(vertex->GetCount(), emit_count, 0, 0);
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
		}
	}
	void Present() {
		device->SwapBuffer();
		swap_index++;
	}
};
}
