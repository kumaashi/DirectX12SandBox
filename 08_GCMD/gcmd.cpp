#include <windows.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <map>
#include <vector>
#include <string>

#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")

enum {
	CMD_NOP,
	CMD_SET_BARRIER,
	CMD_SET_RENDER_TARGET,
	CMD_SET_TEXTURE,
	CMD_SET_VERTEX,
	CMD_SET_INDEX,
	CMD_SET_CONSTANT,
	CMD_SET_SHADER,
	CMD_CLEAR,
	CMD_DRAW_INDEX,
	CMD_QUIT,
	CMD_MAX,
};

struct matrix4x4 {
	float data[16];
};

struct vector4 {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
	};
};

struct cmd {
	int type;
	std::string name;
	struct rect_t {
		int x, y, w, h;
	};
	union {
		struct set_barrier_t {
			bool to_present;
			bool to_rendertarget;
			bool to_texture;
		} set_barrier;

		struct set_render_target_t {
			int fmt;
			rect_t rect;
		} set_render_target;

		struct set_texture_t {
			int fmt;
			int slot;
			void *data;
			size_t size;
			rect_t rect;
		} set_texture;

		struct set_vertex_t {
			void *data;
			size_t size;
			size_t stride_size;
		} set_vertex;

		struct set_index_t {
			void *data;
			size_t size;
		} set_index;

		struct set_constant_t {
			int slot;
			void *data;
			size_t size;
		} set_constant;

		struct set_shader_t {
			bool is_update;
		} set_shader;
		
		struct clear_t {
			vector4 color;
		} clear;

		struct draw_index_t {
			int start;
			int count;
		} draw_index;
	};
	
	void print() {
		printf("cmd:name=%s:\t\t\t", name.c_str());
		switch(type) {
		case CMD_CLEAR:
			printf("CMD_CLEAR :%f %f %f %f\n", clear.color.x, clear.color.y, clear.color.z, clear.color.w);
			break;
		case CMD_SET_BARRIER:
			printf("CMD_SET_BARRIER :");
			printf("%d %d %d\n", set_barrier.to_present, set_barrier.to_rendertarget, set_barrier.to_texture);
			break;
		case CMD_SET_RENDER_TARGET:
			printf("CMD_SET_RENDER_TARGET :");
			printf("rect.x=%d rect.x=%d rect.x=%d rect.x=%d : fmt=%d\n",
				set_render_target.rect.x, set_render_target.rect.y, set_render_target.rect.w, set_render_target.rect.h, set_render_target.fmt);
			break;
		case CMD_SET_TEXTURE:
			printf("CMD_SET_TEXTURE :");
			printf("%d %d %d %d : slot=%d, fmt=%d, data=%p, size=%p\n",
				set_texture.rect.x, set_texture.rect.y, set_texture.rect.w, set_texture.rect.h, set_texture.slot, set_texture.fmt, set_texture.data, set_texture.size);
			break;
		case CMD_SET_VERTEX:
			printf("CMD_SET_VERTEX :");
			printf("data=%p, size=%zu\n", set_vertex.data, set_vertex.size);
			break;
		case CMD_SET_INDEX:
			printf("CMD_SET_INDEX :");
			printf("data=%p, size=%zu\n", set_index.data, set_index.size);
			break;
		case CMD_SET_CONSTANT:
			printf("CMD_SET_CONSTANT :");
			printf("slot=%d, data=%p, size=%zu\n", set_constant.slot, set_constant.data, set_constant.size);
			break;
		case CMD_SET_SHADER:
			printf("CMD_SET_SHADER :");
			printf("is_update=%d\n", set_shader.is_update);
			break;
		case CMD_DRAW_INDEX:
			printf("CMD_DRAW_INDEX :");
			printf("start=%d, count=%d\n", draw_index.start, draw_index.count);
			break;
		case CMD_MAX:
			break;
		}
	}
};

ID3D12Resource * CreateResource(std::string name, ID3D12Device *dev, int w, int h, DXGI_FORMAT fmt,
	D3D12_RESOURCE_FLAGS flags, BOOL is_upload = FALSE, void *data = 0, size_t size = 0)
{
	ID3D12Resource *res = nullptr;
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, UINT64(w), UINT(h), 1, 1, fmt,
		{1, 0}, D3D12_TEXTURE_LAYOUT_UNKNOWN, flags
	};
	D3D12_HEAP_PROPERTIES hprop = {
		D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1,
	};

	if (is_upload) {
		hprop.Type = D3D12_HEAP_TYPE_UPLOAD;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.MipLevels = 1;
	}
	auto state = D3D12_RESOURCE_STATE_GENERIC_READ;
	auto hr = dev->CreateCommittedResource(
		&hprop, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&res));
	if (hr)
		printf("%s : ERR name=%s: w=%d, h=%d, flags=%08X, hr=%08X\n", __FUNCTION__, name.c_str(), w, h, flags, hr);
	else
		printf("%s : INFO name=%s: w=%d, h=%d, flags=%08X, hr=%08X\n", __FUNCTION__, name.c_str(), w, h, flags, hr);
	if (res && is_upload && data) {
		UINT8 *dest = nullptr;
		res->Map(0, NULL, reinterpret_cast<void **>(&dest));
		if (dest) {
			printf("%s : INFO UPLOAD name=%s: w=%d, h=%d, data=%p, size=%p\n",
				__FUNCTION__, name.c_str(), w, h, data, size);
			memcpy(dest, data, size);
			res->Unmap(0, NULL);
		} else {
			printf("%s : cant map\n", __FUNCTION__);
		}
	}
	return res;
}

D3D12_RESOURCE_BARRIER GetBarrier(ID3D12Resource *res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = res;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return barrier;
}

D3D12_SHADER_BYTECODE CreateShaderFromFile(
	std::string fstr, std::string entry, std::string profile, std::vector<uint8_t> &shader_code)
{
	ID3DBlob *blob = nullptr;
	ID3DBlob *blob_err = nullptr;
	ID3DBlob *blob_sig = nullptr;

	std::vector<WCHAR> wfname;
	UINT flags = 0;
	for (int i = 0; i < fstr.length(); i++)
		wfname.push_back(fstr[i]);
	wfname.push_back(0);
	D3DCompileFromFile(&wfname[0], NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry.c_str(), profile.c_str(), flags, 0, &blob, &blob_err);
	if (blob_err) {
		printf("%s:\n%s\n", __FUNCTION__, (char *)blob_err->GetBufferPointer());
		blob_err->Release();
	}
	if (!blob && !blob_err)
		printf("File Not found : %s\n", fstr.c_str());
	if (!blob)
		return {nullptr, 0};
	shader_code.resize(blob->GetBufferSize());
	memcpy(shader_code.data(), blob->GetBufferPointer(), blob->GetBufferSize());
	if (blob)
		blob->Release();
	return {shader_code.data(), shader_code.size()};
}

void PresentGraphics(std::vector<cmd> & vcmd, HWND hwnd, UINT w, UINT h, UINT num, UINT heapcount, UINT slotmax)
{
	struct DeviceBuffer {
		ID3D12CommandAllocator *cmdalloc = nullptr;
		ID3D12GraphicsCommandList *cmdlist = nullptr;
		ID3D12Fence *fence = nullptr;
		std::vector<ID3D12Resource *> vscratch;
		uint64_t value = 0;
	};
	static std::vector<DeviceBuffer> devicebuffer;
	static ID3D12Device *dev = nullptr;
	static ID3D12CommandQueue *queue = nullptr;
	static IDXGISwapChain3 *swapchain = nullptr;
	static ID3D12DescriptorHeap *heap_rtv = nullptr;
	static ID3D12DescriptorHeap *heap_dsv = nullptr;
	static ID3D12DescriptorHeap *heap_shader = nullptr;
	static ID3D12RootSignature *rootsig = nullptr;
	static std::map<std::string, ID3D12Resource *> mres;
	static std::map<std::string, ID3D12PipelineState *> mpstate;
	static std::map<std::string, uint64_t> mcpu_handle;
	static std::map<std::string, uint64_t> mgpu_handle;
	static uint64_t handle_index_rtv = 0;
	static uint64_t handle_index_dsv = 0;
	static uint64_t handle_index_shader = 0;
	static uint64_t deviceindex = 0;
	static uint64_t frame_count = 0;

	if(dev == nullptr) {
		D3D12_COMMAND_QUEUE_DESC cqdesc = {};
		D3D12_DESCRIPTOR_HEAP_DESC dhdesc_rtv = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, heapcount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
		D3D12_DESCRIPTOR_HEAP_DESC dhdesc_dsv = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, heapcount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
		D3D12_DESCRIPTOR_HEAP_DESC dhdesc_shader = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, heapcount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };

		D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&dev));
		dev->CreateCommandQueue(&cqdesc, IID_PPV_ARGS(&queue));
		dev->CreateDescriptorHeap(&dhdesc_rtv, IID_PPV_ARGS(&heap_rtv));
		dev->CreateDescriptorHeap(&dhdesc_dsv, IID_PPV_ARGS(&heap_dsv));
		dev->CreateDescriptorHeap(&dhdesc_shader, IID_PPV_ARGS(&heap_shader));

		IDXGIFactory4 *factory = nullptr;
		IDXGISwapChain *temp = nullptr;
		DXGI_SWAP_CHAIN_DESC desc = {
			{
				w, h, {0, 0},
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				DXGI_MODE_SCALING_UNSPECIFIED
			},
			{1, 0},
			DXGI_USAGE_RENDER_TARGET_OUTPUT,
			num, hwnd, TRUE, DXGI_SWAP_EFFECT_FLIP_DISCARD,
			DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
		};
		CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		factory->CreateSwapChain(queue, &desc, &temp);
		temp->QueryInterface(IID_PPV_ARGS(&swapchain));
		temp->Release();
		factory->Release();

		devicebuffer.resize(num);
		for(auto & x : devicebuffer) {
			dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&x.cmdalloc));
			dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, x.cmdalloc, nullptr, IID_PPV_ARGS(&x.cmdlist));
			dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&x.fence));
			x.cmdlist->Close();
		}
		
		for(int i = 0 ; i < num; i++) {
			ID3D12Resource *res = nullptr;
			swapchain->GetBuffer(i, IID_PPV_ARGS(&res));
			mres["backbuffer" + std::to_string(i)] = res;
		}

		D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
		std::vector<D3D12_ROOT_PARAMETER> vroot_param;
		std::vector<D3D12_DESCRIPTOR_RANGE> vdesc_range;
		D3D12_ROOT_PARAMETER root_param = {};
		root_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root_param.DescriptorTable.NumDescriptorRanges = 1;
		
		for(UINT i = 0 ; i < slotmax; i++) {
			vdesc_range.push_back({D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND});
			vdesc_range.push_back({D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND});
		}

		for(auto & x : vdesc_range) {
			root_param.DescriptorTable.pDescriptorRanges = &x;
			vroot_param.push_back(root_param);
		}

		D3D12_STATIC_SAMPLER_DESC sampler[2];
		const D3D12_STATIC_SAMPLER_DESC default_sampler = {
			D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			0.0f, 0.0f, D3D12_COMPARISON_FUNC_NEVER, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 0.0f, D3D12_FLOAT32_MAX,
			0, 0, D3D12_SHADER_VISIBILITY_ALL,
		};
		
		sampler[0] = default_sampler;
		sampler[0].ShaderRegister = 0;
		sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler[1] = default_sampler;
		sampler[1].ShaderRegister = 1;
		sampler[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		root_signature_desc.pStaticSamplers = sampler;
		root_signature_desc.NumStaticSamplers = _countof(sampler);

		root_signature_desc.NumParameters = vroot_param.size();
		root_signature_desc.pParameters = vroot_param.data();
		ID3DBlob *perrblob = nullptr;
		ID3DBlob *signature = nullptr;

		auto hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &perrblob);
		if (hr && perrblob) {
			printf("ERR : Failed D3D12SerializeRootSignature:\n%s\n", (char *)perrblob->GetBufferPointer());
			exit(1);
		}
		
		hr = dev->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootsig));
		if(perrblob) perrblob->Release();
		if(signature) signature->Release();
	};
	
	if(dev->GetDeviceRemovedReason()) {
		printf("!!!!!!!!!!!!!!!!Device Lost frame_count=%p\n", frame_count);
		Sleep(1000);
	}
	
	std::map<std::string, D3D12_RESOURCE_TRANSITION_BARRIER> mbarrier;
	deviceindex = swapchain->GetCurrentBackBufferIndex();

	auto & ref = devicebuffer[deviceindex];
	if (ref.fence->GetCompletedValue() != ref.value) {
		auto hevent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
		queue->Signal(ref.fence, ref.value);
		ref.fence->SetEventOnCompletion(ref.value, hevent);
		WaitForSingleObject(hevent, INFINITE);
		CloseHandle(hevent);
	}
	
	for(auto & scratch : ref.vscratch)
		scratch->Release();
	ref.vscratch.clear();

	if(hwnd == nullptr) {
		auto release = [](auto & x) {
			if(x) x->Release();
			x = nullptr;
		};
		auto mrelease = [=](auto & m) {
			for(auto & p : m) {
				if(p.second)
					printf("%s : release=%s\n", __FUNCTION__, p.first.c_str());
				release(p.second);
			}
			m.clear();
		};
		for(auto & ref : devicebuffer) {
			release(ref.fence);
			release(ref.cmdlist);
			release(ref.cmdalloc);
		}
		mrelease(mres);
		mrelease(mpstate);
		release(rootsig);
		release(heap_shader);
		release(heap_dsv);
		release(heap_rtv);
		release(swapchain);
		release(queue);
		release(dev);
		return;
	}

	ref.cmdalloc->Reset();
	ref.cmdlist->Reset(ref.cmdalloc, 0);
	ref.cmdlist->SetGraphicsRootSignature(rootsig);
	ref.cmdlist->SetDescriptorHeaps(1, &heap_shader);
	for(auto & c : vcmd) {
		auto type = c.type;
		auto name = c.name;
		auto res = mres[name];
		auto pstate = mpstate[name];

		//CMD_SET_BARRIER
		if(type == CMD_SET_BARRIER) {
			D3D12_RESOURCE_TRANSITION_BARRIER tb {};
			tb.pResource = nullptr;
			if(c.set_barrier.to_present) {
				tb.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				tb.StateAfter = D3D12_RESOURCE_STATE_COMMON;
			}
			if(c.set_barrier.to_texture) {
				tb.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				tb.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			if(c.set_barrier.to_rendertarget) {
				tb.StateBefore = D3D12_RESOURCE_STATE_COMMON;
				tb.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
			mbarrier[name] = tb;
		}

		//CMD_SET_RENDER_TARGET
		if(type == CMD_SET_RENDER_TARGET) {
			auto x = c.set_render_target.rect.x;
			auto y = c.set_render_target.rect.y;
			auto w = c.set_render_target.rect.w;
			auto h = c.set_render_target.rect.h;
			auto cpu_handle = heap_rtv->GetCPUDescriptorHandleForHeapStart();
			auto fmt = DXGI_FORMAT_R8G8B8A8_UNORM;

			if(res == nullptr) {
				D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
				res = CreateResource(name, dev, w, h, fmt, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
				mres[name] = res;
			}

			if(mcpu_handle.count(name) == 0) {
				auto temp = cpu_handle;
				D3D12_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.Texture2D.MipSlice = 0;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				temp.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * handle_index_rtv;
				dev->CreateRenderTargetView(res, &desc, temp);
				mcpu_handle[name] = handle_index_rtv++;
			};

			if(mbarrier.count(name)) {
				D3D12_RESOURCE_BARRIER barrier = GetBarrier(nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);
				barrier.Transition = mbarrier[name];
				barrier.Transition.pResource = mres[name];
				ref.cmdlist->ResourceBarrier(1, &barrier);
			}
			mbarrier.erase(name);
			
			auto cpu_index = mcpu_handle[name];
			cpu_handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * cpu_index;
			D3D12_VIEWPORT viewport = { FLOAT(x), FLOAT(y), FLOAT(w), FLOAT(h), 0.0f, 1.0f };
			D3D12_RECT rect = { x, y, w, h };
			ref.cmdlist->RSSetViewports(1, &viewport);
			ref.cmdlist->RSSetScissorRects(1, &rect);
			ref.cmdlist->OMSetRenderTargets(1, &cpu_handle, FALSE, nullptr); //TODO DSV
		}

		//CMD_SET_TEXTURE
		if(type == CMD_SET_TEXTURE) {
			auto w = c.set_texture.rect.w;
			auto h = c.set_texture.rect.h;
			auto cpu_handle = heap_shader->GetCPUDescriptorHandleForHeapStart();
			auto gpu_handle = heap_shader->GetGPUDescriptorHandleForHeapStart();
			auto slot = c.set_texture.slot;
			auto fmt = DXGI_FORMAT_R8G8B8A8_UNORM;

			if(res == nullptr) {
				D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
				res = CreateResource(name, dev, w, h, fmt, D3D12_RESOURCE_FLAG_NONE);
				auto scratch = CreateResource(name, dev, c.set_texture.size, 1,
					DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE, TRUE, c.set_texture.data, c.set_texture.size);
				ref.vscratch.push_back(scratch);
				mres[name] = res;

				D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
				D3D12_TEXTURE_COPY_LOCATION dest = {};
				D3D12_TEXTURE_COPY_LOCATION src = {};
				D3D12_RESOURCE_DESC desc_res = res->GetDesc();
				UINT64 total_bytes = 0;
				UINT subres_index = 0;

				dev->GetCopyableFootprints(&desc_res, subres_index, 1, 0, &footprint, nullptr, nullptr, &total_bytes);
				dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				dest.pResource = res;
				src.pResource = scratch;

				dest.SubresourceIndex = subres_index;
				src.PlacedFootprint = footprint;
				ref.cmdlist->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr );
				D3D12_RESOURCE_BARRIER barrier = GetBarrier(res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
				ref.cmdlist->ResourceBarrier(1, &barrier);
			}
			if(mgpu_handle.count(name) == 0) {
				D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				desc.Texture2D.MipLevels = 1;
				cpu_handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * handle_index_shader;
				dev->CreateShaderResourceView(res, &desc, cpu_handle);
				mgpu_handle[name] = handle_index_shader++;
			}
			D3D12_RESOURCE_DESC desc_res = res->GetDesc();
			if(mbarrier.count(name) && (desc_res.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ) {
				D3D12_RESOURCE_BARRIER barrier = GetBarrier(nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);
				barrier.Transition = mbarrier[name];
				barrier.Transition.pResource = mres[name];
				ref.cmdlist->ResourceBarrier(1, &barrier);
			}
			mbarrier.erase(name);
			auto gpu_index = mgpu_handle[name];
			gpu_handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * gpu_index;
			ref.cmdlist->SetGraphicsRootDescriptorTable((slot * 2) + 0, gpu_handle);
		}

		//CMD_SET_VERTEX
		if(type == CMD_SET_VERTEX) {
			if(res == nullptr) {
				res = CreateResource(name, dev, c.set_vertex.size, 1,
					DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE, TRUE, c.set_vertex.data, c.set_vertex.size);
				mres[name] = res;
			}
			D3D12_VERTEX_BUFFER_VIEW view = {
				res->GetGPUVirtualAddress(), UINT(c.set_vertex.size), UINT(c.set_vertex.stride_size)
			};
			ref.cmdlist->IASetVertexBuffers(0, 1, &view);
			ref.cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		}

		//CMD_SET_INDEX
		if(type == CMD_SET_INDEX) {
			if(res == nullptr) {
				res = CreateResource(name, dev, c.set_vertex.size, 1,
					DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE, TRUE, c.set_index.data, c.set_index.size);
				mres[name] = res;
			}
			D3D12_INDEX_BUFFER_VIEW view = {
				res->GetGPUVirtualAddress(), UINT(c.set_index.size), DXGI_FORMAT_R32_UINT
			};
			ref.cmdlist->IASetIndexBuffer(&view);
		}

		//CMD_SET_SHADER
		if(type == CMD_SET_SHADER) {
			if(pstate == nullptr || c.set_shader.is_update) {
				if(pstate)
					pstate->Release();
				pstate = nullptr;
				mpstate[name] = nullptr;
				
				std::vector<uint8_t> vs;
				std::vector<uint8_t> ps;
				D3D12_GRAPHICS_PIPELINE_STATE_DESC gpstate_desc = {};
				D3D12_INPUT_ELEMENT_DESC iedesc = {
					"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				};
				gpstate_desc.InputLayout.pInputElementDescs = &iedesc;
				gpstate_desc.InputLayout.NumElements = 1;
				for(auto & bs : gpstate_desc.BlendState.RenderTarget) {
					bs.BlendEnable = FALSE;
					bs.LogicOpEnable = FALSE;
					bs.SrcBlend = D3D12_BLEND_SRC_ALPHA;
					bs.DestBlend = D3D12_BLEND_INV_DEST_ALPHA;
					bs.BlendOp = D3D12_BLEND_OP_ADD;
					bs.SrcBlendAlpha = D3D12_BLEND_ONE;
					bs.DestBlendAlpha = D3D12_BLEND_ZERO;
					bs.BlendOpAlpha = D3D12_BLEND_OP_ADD;
					bs.LogicOp = D3D12_LOGIC_OP_XOR;
					bs.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				}
				gpstate_desc.NumRenderTargets = _countof(gpstate_desc.BlendState.RenderTarget);
				gpstate_desc.pRootSignature = rootsig;
				gpstate_desc.VS = CreateShaderFromFile(name, "VSMain", "vs_5_0", vs);
				gpstate_desc.PS = CreateShaderFromFile(name, "PSMain", "ps_5_0", ps);
				gpstate_desc.SampleDesc.Count = 1;
				gpstate_desc.SampleMask = UINT_MAX;
				gpstate_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
				gpstate_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
				gpstate_desc.RasterizerState.DepthClipEnable = TRUE;
				gpstate_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				
				for(auto & fmt : gpstate_desc.RTVFormats)
					fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
				
				if(!vs.empty() && !ps.empty()) {
					auto status = dev->CreateGraphicsPipelineState(&gpstate_desc, IID_PPV_ARGS(&pstate));
					if(pstate)
						mpstate[name] = pstate;
					else
						printf("Error CreateGraphicsPipelineState : %s : status=%p\n", name.c_str(), status);
				} else {
					printf("Compile Error %s\n", name.c_str());
				}
			}
			if(pstate)
				ref.cmdlist->SetPipelineState(pstate);
			else
				Sleep(500);
		}

		//CMD_CLEAR
		if(type == CMD_CLEAR) {
			auto cpu_handle = heap_rtv->GetCPUDescriptorHandleForHeapStart();
			auto index = mcpu_handle[name];
			cpu_handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * index;
			ref.cmdlist->ClearRenderTargetView(cpu_handle, c.clear.color.data, 0, NULL);
		}

		//CMD_SET_CONSTANT
		if(type == CMD_SET_CONSTANT) {
			auto cpu_handle = heap_shader->GetCPUDescriptorHandleForHeapStart();
			auto gpu_handle = heap_shader->GetGPUDescriptorHandleForHeapStart();
			auto slot = c.set_constant.slot;
			if(res == nullptr) {
				res = CreateResource(name, dev, c.set_constant.size, 1,
					DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE, TRUE, c.set_constant.data, c.set_constant.size);
				mres[name] = res;
			}
			if(mgpu_handle.count(name) == 0) {
				D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
				D3D12_RESOURCE_DESC desc_res = res->GetDesc();
				desc.SizeInBytes = (desc_res.Width + 255) & ~255;
				desc.BufferLocation = res->GetGPUVirtualAddress();
				cpu_handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * handle_index_shader;
				dev->CreateConstantBufferView(&desc, cpu_handle);
				mgpu_handle[name] = handle_index_shader++;
			}
			auto gpu_index = mgpu_handle[name];
			gpu_handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * gpu_index;
			ref.cmdlist->SetGraphicsRootDescriptorTable((slot * 2) + 1, gpu_handle);
			{
				UINT8 *dest = nullptr;
				res->Map(0, NULL, reinterpret_cast<void **>(&dest));
				if (dest) {
					memcpy(dest, c.set_constant.data, c.set_constant.size);
					res->Unmap(0, NULL);
				} else {
					printf("%s : cant map\n", __FUNCTION__);
				}
			}
		}
		
		//CMD_DRAW_INDEX
		if(type == CMD_DRAW_INDEX) {
			UINT IndexCountPerInstance = c.draw_index.count;
			UINT InstanceCount = 1;
			UINT StartIndexLocation = 0;
			INT  BaseVertexLocation = 0;
			UINT StartInstanceLocation = 0;
			ref.cmdlist->DrawIndexedInstanced(
				IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
		}
	}
	for(auto & tb : mbarrier) {
		D3D12_RESOURCE_BARRIER barrier = GetBarrier(nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);
		barrier.Transition = tb.second;
		barrier.Transition.pResource = mres[tb.first];
		ref.cmdlist->ResourceBarrier(1, &barrier);
	}
	ref.cmdlist->Close();
	ID3D12CommandList *pplists[] = {
		ref.cmdlist,
	};
	queue->ExecuteCommandLists(1, pplists);
	swapchain->Present(1, 0);
	ref.value = frame_count++;
}


static LRESULT WINAPI
MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_SYSCOMMAND:
		switch ((wParam & 0xFFF0)) {
		case SC_MONITORPOWER:
		case SC_SCREENSAVE:
			return 0;
		default:
			break;
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_IME_SETCONTEXT:
		lParam &= ~ISC_SHOWUIALL;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND
InitWindow(const char *name, int w, int h)
{
	HINSTANCE instance = GetModuleHandle(NULL);
	auto style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	auto ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	RECT rc = {0, 0, w, h};
	WNDCLASSEX twc = {
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, instance,
		LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)GetStockObject(BLACK_BRUSH), NULL, name, NULL
	};

	RegisterClassEx(&twc);
	AdjustWindowRectEx(&rc, style, FALSE, ex_style);
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	HWND hwnd = CreateWindowEx(
		ex_style, name, name, style,
		(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
		rc.right, rc.bottom, NULL, NULL, instance, NULL);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(hwnd);
	return hwnd;
};

int
Update()
{
	MSG msg;
	int is_active = 1;

	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			is_active = 0;
			break;
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return is_active;
}


void SetBarrierToPresent(std::vector<cmd> & vcmd, std::string name)
{
	cmd c;
	c.type = CMD_SET_BARRIER;
	c.name = name;
	c.set_barrier.to_present = true;
	c.set_barrier.to_rendertarget = false;
	c.set_barrier.to_texture = false;
	vcmd.push_back(c);
}

void SetBarrierToRenderTarget(std::vector<cmd> & vcmd, std::string name)
{
	cmd c;
	c.type = CMD_SET_BARRIER;
	c.name = name;
	c.set_barrier.to_present = false;
	c.set_barrier.to_rendertarget = true;
	c.set_barrier.to_texture = false;
	vcmd.push_back(c);
}

void SetBarrierToTexture(std::vector<cmd> & vcmd, std::string name)
{
	cmd c;
	c.type = CMD_SET_BARRIER;
	c.name = name;
	c.set_barrier.to_present = false;
	c.set_barrier.to_rendertarget = false;
	c.set_barrier.to_texture = true;
	vcmd.push_back(c);
}

void SetRenderTarget(std::vector<cmd> & vcmd, std::string name, int w, int h)
{
	SetBarrierToRenderTarget(vcmd, name);

	cmd c;
	c.type = CMD_SET_RENDER_TARGET;
	c.name = name;
	c.set_render_target.fmt = 0;
	c.set_render_target.rect.x = 0;
	c.set_render_target.rect.y = 0;
	c.set_render_target.rect.w = w;
	c.set_render_target.rect.h = h;
	vcmd.push_back(c);
}

void SetTexture(std::vector<cmd> & vcmd, std::string name, int slot, int w = 0, int h = 0, void *data = nullptr, size_t size = 0)
{
	SetBarrierToTexture(vcmd, name);

	cmd c;
	c.type = CMD_SET_TEXTURE;
	c.name = name;
	c.set_texture.fmt = 0;
	c.set_texture.slot = slot;
	c.set_texture.data = data;
	c.set_texture.size = size;
	c.set_texture.rect.x = 0;
	c.set_texture.rect.y = 0;
	c.set_texture.rect.w = w;
	c.set_texture.rect.h = h;
	vcmd.push_back(c);
}

void SetVertex(std::vector<cmd> & vcmd, std::string name, void *data, size_t size, size_t stride_size)
{
	cmd c;
	c.type = CMD_SET_VERTEX;
	c.name = name;
	c.set_vertex.data = data;
	c.set_vertex.size = size;
	c.set_vertex.stride_size = stride_size;
	vcmd.push_back(c);
}

void SetIndex(std::vector<cmd> & vcmd, std::string name, void *data, size_t size)
{
	cmd c;
	c.type = CMD_SET_INDEX;
	c.name = name;
	c.set_index.data = data;
	c.set_index.size = size;
	vcmd.push_back(c);
}

void SetConstant(std::vector<cmd> & vcmd, std::string name, int slot, void *data, size_t size)
{
	cmd c;
	c.type = CMD_SET_CONSTANT;
	c.name = name;
	c.set_constant.slot = slot;
	c.set_constant.data = data;
	c.set_constant.size = size;
	vcmd.push_back(c);
}

void SetShader(std::vector<cmd> & vcmd, std::string name, bool is_update)
{
	cmd c;
	c.type = CMD_SET_SHADER;
	c.name = name;
	c.set_shader.is_update = is_update;
	vcmd.push_back(c);
}

void ClearRenderTarget(std::vector<cmd> & vcmd, std::string name, vector4 col)
{
	cmd c;
	c.type = CMD_CLEAR;
	c.name = name;
	c.clear.color = col;
	vcmd.push_back(c);
}

void DrawIndex(std::vector<cmd> & vcmd, std::string name, int start, int count)
{
	cmd c;
	c.type = CMD_DRAW_INDEX;
	c.name = name;
	c.draw_index.start = start;
	c.draw_index.count = count;
	vcmd.push_back(c);
}

void DebugPrint(std::vector<cmd> & vcmd) {
	for(auto & c : vcmd)
		c.print();
}

int main() {
	enum {
		Width = 1280,
		Height = 720,
		BufferMax = 2,
		ShaderSlotMax = 8,
		ResourceMax = 1024,
	};
	vector4 vtx[] = {
		{-1, 1, 0, 1},
		{-1,-1, 0, 1},
		{ 1, 1, 0, 1},
		{ 1,-1, 0, 1},
	};
	uint32_t idx[] = {
		0, 1, 2,
		2, 1, 3,
	};
	auto hwnd = InitWindow("test", Width, Height);
	int index = 0;
	static std::vector<uint32_t> vtex;
	for(int y = 0  ; y < 256; y++) {
		for(int x = 0  ; x < 256; x++) {
			vtex.push_back((x ^ y) * 1110);
		}
	}

	struct constdata {
		vector4 color;
		vector4 misc;
	};
	constdata cdata;

	std::vector<cmd> vcmd;

	SetTexture(vcmd, "testtex", 0, 256, 256, vtex.data(), vtex.size() * sizeof(uint32_t));
	uint64_t frame = 0;
	auto beforeoffscreenname = "offscreen" + std::to_string(1);
	while(Update()) {
		bool is_update = false;
		if(GetAsyncKeyState(VK_F5) & 0x0001) {
			is_update = true;
		}
		auto buffer_index = frame % BufferMax;
		cdata.color.data[0] = 1.0;
		cdata.color.data[1] = 0.0;
		cdata.color.data[2] = 1.0;
		cdata.color.data[3] = 1.0;
		cdata.misc.data[0] = float(frame) / 1000.0f;
		cdata.misc.data[1] = 0.0;
		cdata.misc.data[2] = 1.0;
		cdata.misc.data[3] = 1.0;
		
		auto indexname = std::to_string(buffer_index);
		auto backbuffername = "backbuffer" + indexname;
		auto offscreenname = "offscreen" + indexname;
		auto constantname = "testconstant" + indexname;
		SetRenderTarget(vcmd, offscreenname, Width, Height);
		ClearRenderTarget(vcmd, offscreenname, {1, float(index & 1), 0, 1});
		if(frame >= 1) {
			SetTexture(vcmd, beforeoffscreenname, 1);
		}
		SetShader(vcmd, "test.hlsl", is_update);
		SetTexture(vcmd, "testtex", 0);
		SetVertex(vcmd, "testvertex", vtx, sizeof(vtx), sizeof(vector4));
		SetIndex(vcmd, "testindex", idx, sizeof(idx));
		SetConstant(vcmd, constantname, 0, &cdata, sizeof(cdata));
		DrawIndex(vcmd, "offscreendraw", 0, _countof(idx));
		
		SetRenderTarget(vcmd, backbuffername, Width, Height);
		ClearRenderTarget(vcmd, backbuffername, {1, float(index & 1), 0, 1});
		SetShader(vcmd, "present.hlsl", is_update);
		SetTexture(vcmd, offscreenname, 0, 0, 0, nullptr, 0);
		SetVertex(vcmd, "testvertex", vtx, sizeof(vtx), sizeof(vector4));
		SetIndex(vcmd, "testindex", idx, sizeof(idx));
		SetConstant(vcmd, constantname, 0, &cdata, sizeof(cdata));
		DrawIndex(vcmd, "presentdraw", 0, _countof(idx));
		SetBarrierToPresent(vcmd, backbuffername);
		PresentGraphics(vcmd, hwnd, Width, Height, BufferMax, ResourceMax, ShaderSlotMax);
		beforeoffscreenname = offscreenname;
		DebugPrint(vcmd);
		vcmd.clear();
		printf("Frame=%d ========================================\n", frame);
		frame++;
	}
	PresentGraphics(vcmd, nullptr, Width, Height, BufferMax, ResourceMax, ShaderSlotMax);
	return 0;
}
