#include "../include/dx12util.h"
#include "../include/dx12cmd.h"
#include "../include/winapp.h"

#include <map>

#define DEBUG_PTR(obj) printf(#obj "=%p\n", obj);

#include <stdio.h>
#include <stdlib.h>
#include <DirectXMath.h>

struct MatrixStack
{
	enum {
		Max = 4,
	};
	unsigned index = 0;
	DirectX::XMMATRIX data[Max];

	MatrixStack()
	{
		Reset();
	}

	auto & Get(int i)
	{
		return data[i];
	}

	auto & GetTop()
	{
		return Get(index);
	}

	void GetTop(float *a)
	{
		*(DirectX::XMMATRIX *)a = GetTop();
	}

	void Reset()
	{
		index = 0;
		for(int i = 0 ; i < Max; i++) {
			auto & m = Get(i);
			m = DirectX::XMMatrixIdentity();
		}
	}

	void Push()
	{
		auto & mb = GetTop();
		index = (index + 1) % Max;
		auto & m = GetTop();
		m = mb;
		printf("%s:index=%d\n", __FUNCTION__, index);
	}

	void Pop()
	{
		index = (index - 1) % Max;
		if(index < 0)
			printf("%s : under flow\n", __FUNCTION__);
		printf("%s:index=%d\n", __FUNCTION__, index);
	}
	
	void Load(DirectX::XMMATRIX a)
	{
		auto & m = GetTop();
		m = a;
	}

	void Load(float *a)
	{
		Load(*(DirectX::XMMATRIX *)a);
	}

	void Mult(DirectX::XMMATRIX a)
	{
		auto & m = GetTop();
		m *= a;
	}

	void Mult(float *a)
	{
		Mult(*(DirectX::XMMATRIX *)a);
	}

	void LoadIdentity()
	{
		Load(DirectX::XMMatrixIdentity());
	}

	void LoadLookAt(
		float px, float py, float pz,
		float ax, float ay, float az,
		float ux, float uy, float uz)
	{
		Load(DirectX::XMMatrixLookAtLH({px, py, pz}, {ax, ay, az}, {ux, uy, uz}));
	}

	void LoadPerspective(float ffov, float faspect, float fnear, float ffar)
	{
		Load(DirectX::XMMatrixPerspectiveFovLH(ffov, faspect, fnear, ffar));
	}
	
	void Translation(float x, float y, float z)
	{
		Mult(DirectX::XMMatrixTranslation(x, y, z));
	}

	void RotateAxis(float x, float y, float z, float angle)
	{
		Mult(DirectX::XMMatrixRotationAxis({x, y, z}, angle));
	}

	void RotateX(float angle)
	{
		Mult(DirectX::XMMatrixRotationX(angle));
	}

	void RotateY(float angle)
	{
		Mult(DirectX::XMMatrixRotationY(angle));
	}

	void RotateZ(float angle)
	{
		Mult(DirectX::XMMatrixRotationZ(angle));
	}
	
	void Scaling(float x, float y, float z, float angle)
	{
		Mult(DirectX::XMMatrixScaling(x, y, z));
	}

	void Transpose()
	{
		Load(DirectX::XMMatrixTranspose(GetTop()));
	}

	void Print(DirectX::XMMATRIX m)
	{
		int i = 0;
		for(auto & v : m.r) {
			for(auto & e : v.m128_f32) {
				if((i % 4) == 0) printf("\n");
				printf("[%02d]%.4f, ", i++, e);
			}
		}
		printf("\n");
	}

	void Print()
	{
		Print(GetTop());
	}

	void PrintAll()
	{
		for(int i = 0; i < Max; i++) {
			Print(Get(i));
		}
	}
};

#if 0 //test
int main() {
	MatrixStack stack;
	stack.LoadPerspective(90.0, 1.25, 0.01, 256.0f);
	stack.Push();
	stack.Load(
		{
			{100.0}, {100.0}, {100.0}, {100.0}, 
			{100.0}, {100.0}, {100.0}, {100.0}, 
			{100.0}, {100.0}, {100.0}, {100.0}, 
			{100.0}, {100.0}, {100.0}, {100.0}, 
		});
	stack.Push();

	stack.LoadLookAt(
		0, 0, -10,
		0, 0, 0,
		0, 1, 0
	);
	stack.Translation(123, 456, 789);

	stack.PrintAll();
	return 0;
}
#endif //0

struct heap_allocator
{
	struct handle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE hcpu;
		D3D12_GPU_DESCRIPTOR_HANDLE hgpu;
	};
	std::vector<handle> vdata;
	ID3D12DescriptorHeap *heap = nullptr;
	int index = 0;
	void term()
	{
		if(heap) heap->Release();
	}
	void init(ID3D12Device *dev, D3D12_DESCRIPTOR_HEAP_TYPE type, int num)
	{
		dx12util::create_desc_heap(dev, num, type, &heap);
		auto hcpu = heap->GetCPUDescriptorHandleForHeapStart();
		auto hgpu = heap->GetGPUDescriptorHandleForHeapStart();
		auto incsize = dev->GetDescriptorHandleIncrementSize(type);
		printf("INFO init\n");
		for(int i = 0 ; i < num; i++) {
			handle d = {hcpu, hgpu};
			vdata.push_back(d);
			hcpu.ptr += incsize;
			hgpu.ptr += incsize;
			printf("d.hgpu.ptr=%p\n", d.hgpu.ptr);
		}
	}

	handle alloc()
	{
		auto ret = vdata[index];
		printf("INFO alloc index=%d\n", index);
		printf("INFO alloc ret.hcpu.ptr=%p\n", ret.hcpu.ptr);
		printf("INFO alloc ret.hgpu.ptr=%p\n", ret.hgpu.ptr);
		index++;
		return ret;
	}
};


int main() {
	using namespace dx12util;
	using namespace dx12cmd;
	ID3D12Device *dev = nullptr;
	ID3D12CommandQueue *queue = nullptr;
	IDXGISwapChain3 *swapchain = nullptr;
	heap_allocator handle_cbv_srv_uav;
	heap_allocator handle_rtv;
	heap_allocator handle_dsv;
	heap_allocator handle_sampler;
	struct FrameContext {
		ID3D12CommandAllocator *cmdalloc = nullptr;
		ID3D12GraphicsCommandList *cmdlist = nullptr;
		ID3D12Resource *res_backbuffer = nullptr;
		ID3D12Fence *fence = nullptr;
		uint64_t value = 0;
		heap_allocator::handle hbackbuffer;
	};
	FrameContext fctx[2];
	std::map<ID3D12Resource *, heap_allocator::handle> mresource;
	enum
	{
		Width = 640,
		Height = 480,
		BufferNum = 2,
		DescMax = 16,
	};
	auto setres2handle = [&](auto res, auto h) {
		mresource[res] = h;
	};
	auto getres2handle = [&](auto res) {
		return mresource[res];
	};
	winapp app;
	app.create("test", Width, Height);
	create_device(app.get_handle(), &dev);
	create_cmdqueue(dev, &queue);
	create_swap_chain(queue, app.get_handle(), Width, Height, BufferNum, &swapchain);
	handle_cbv_srv_uav.init(dev, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DescMax);
	handle_rtv.init(dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DescMax);
	handle_dsv.init(dev, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DescMax);
	handle_sampler.init(dev, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DescMax);
	for(int i = 0 ; i < BufferNum; i++)
	{
		auto & c = fctx[i];
		c.hbackbuffer = handle_rtv.alloc();
		create_cmdallocator(dev, &c.cmdalloc);
		create_cmdlist(dev, c.cmdalloc, &c.cmdlist);
		create_fence(dev, &c.fence);
		get_buffer_from_swapchain(swapchain, i, &c.res_backbuffer);
		setres2handle(c.res_backbuffer, c.hbackbuffer);
		create_rtv(dev, c.res_backbuffer, c.hbackbuffer.hcpu);
	}

	for(uint64_t frame_count = 0; app.update() ; frame_count++)
	{
		float ccol[2][4] = {
			{1, 0, 0, 1},
			{0, 0, 1, 1},
		};
		auto idx = frame_count % BufferNum;
		auto & ref = fctx[idx];
		auto cmdlist = ref.cmdlist;
		auto cmdalloc = ref.cmdalloc;
		auto fence = ref.fence;
		auto res_backbuffer = ref.res_backbuffer;
		auto hbackbuffer = ref.hbackbuffer;
		if (fence->GetCompletedValue() != ref.value) {
			auto hevent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
			queue->Signal(fence, ref.value);
			fence->SetEventOnCompletion(ref.value, hevent);
			WaitForSingleObject(hevent, INFINITE);
			CloseHandle(hevent);
			printf("Wait\n");
		}
		cmd_reset(cmdlist, cmdalloc);
		cmd_res_barrier(cmdlist, res_backbuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto chandle = hbackbuffer.hcpu;
		cmdlist->OMSetRenderTargets(1, &chandle, FALSE, nullptr);
		cmdlist->ClearRenderTargetView(chandle, ccol[idx], 0, NULL);
		cmd_res_barrier(cmdlist, res_backbuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COMMON);


			
		cmd_exec(cmdlist, queue, fence, frame_count);
		swapchain->Present(0, 0);
		
		ref.value = frame_count;
		printf("frame_count=%p\n", frame_count);
	}
	return 0;
}