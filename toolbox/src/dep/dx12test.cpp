#include "../include/dx12util.h"
#include "../include/winapp.h"

#define DEBUG_PTR(obj) printf(#obj "=%p\n", obj);


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
	winapp app;
	enum
	{
		Width = 640,
		Height = 480,
		BufferNum = 2,
		DescMax = 16,
	};
	app.create("test", Width, Height);
	using namespace dx12util;
	std::vector<IUnknown *> vunknown;
	ID3D12Device *dev = nullptr;
	ID3D12CommandQueue *queue = nullptr;
	IDXGISwapChain3 *swapchain = nullptr;
	heap_allocator handle_cbv_srv_uav;
	heap_allocator handle_rtv;
	heap_allocator handle_dsv;
	heap_allocator handle_sampler;
	struct FrameCommand {
		ID3D12CommandAllocator *cmdalloc = nullptr;
		ID3D12GraphicsCommandList *cmdlist = nullptr;
		ID3D12Resource *res_backbuffer = nullptr;
		ID3D12Fence *fence = nullptr;
		heap_allocator::handle hbackbuffer;
	};
	FrameCommand fcmd[2];
	
	create_device(app.get_handle(), &dev);
	create_cmdqueue(dev, &queue);
	create_swap_chain(
		queue, app.get_handle(), Width, Height, BufferNum, &swapchain);
	handle_cbv_srv_uav.init(
		dev, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DescMax);
	handle_rtv.init(
		dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DescMax);
	handle_dsv.init(
		dev, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DescMax);
	handle_sampler.init(
		dev, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DescMax);

	DEBUG_PTR(dev);
	DEBUG_PTR(queue);
	for(int i = 0 ; i < BufferNum; i++)
	{
		auto & c = fcmd[i];
		c.hbackbuffer = handle_rtv.alloc();
		create_cmdallocator(dev, &c.cmdalloc);
		create_cmdlist(dev, c.cmdalloc, &c.cmdlist);
		create_fence(dev, &c.fence);
		get_buffer_from_swapchain(swapchain, i, &c.res_backbuffer);
		DEBUG_PTR(c.cmdalloc);
		DEBUG_PTR(c.cmdlist);
		DEBUG_PTR(c.res_backbuffer);
		create_rtv(dev, c.res_backbuffer, c.hbackbuffer.hcpu);
	}
	while (app.update())
	{

	}
	for (auto &a : vunknown)
	{
		a->Release();
	}

	return 0;
}