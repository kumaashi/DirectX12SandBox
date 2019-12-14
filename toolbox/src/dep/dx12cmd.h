#pragma once

#include "dx12util.h"

namespace dx12cmd {

using namespace dx12util;

inline int
cmd_reset(
	ID3D12GraphicsCommandList *cmdlist, 
	ID3D12CommandAllocator *cmdallocator)
{
	cmdallocator->Reset();
	cmdlist->Reset(cmdallocator, 0);
	return (0);
}

inline int
cmd_root_resources(
	ID3D12GraphicsCommandList *cmdlist,
	ID3D12RootSignature *root_sig,
	ID3D12DescriptorHeap **heaplist,
	size_t heaplist_count,
	D3D12_GPU_DESCRIPTOR_HANDLE *gpuhandles,
	size_t gpuhandles_count)
{
	cmdlist->SetGraphicsRootSignature(root_sig);
	cmdlist->SetDescriptorHeaps(heaplist_count, heaplist);
	for (int i = 0 ; i < heaplist_count; i++) {
		cmdlist->SetGraphicsRootDescriptorTable(
			i, gpuhandles[i]);
	}
	return (0);
}

inline int
cmd_res_barrier(
	ID3D12GraphicsCommandList *cmdlist,
	ID3D12Resource *res,
	D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = res;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdlist->ResourceBarrier(1, &barrier);
	return (0);
}

inline int
cmd_res_copy(
    ID3D12Device *dev, ID3D12GraphicsCommandList *cmdlist,
    ID3D12Resource *res_dest, ID3D12Resource *res_src, int subres_index)
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT  footprint;
	D3D12_RESOURCE_DESC desc_res = res_dest->GetDesc();
	D3D12_TEXTURE_COPY_LOCATION dest = {};
	D3D12_TEXTURE_COPY_LOCATION src = {};
	UINT64 total_bytes = 0;

	dev->GetCopyableFootprints( &desc_res, subres_index, 1, 0,
		&footprint, nullptr, nullptr, &total_bytes);
	dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest.pResource = res_dest;

	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.pResource = res_src;
	
	dest.SubresourceIndex = subres_index;
	src.PlacedFootprint = footprint;

	cmdlist->CopyTextureRegion( &dest, 0, 0, 0, &src, nullptr );
	cmd_res_barrier(cmdlist, res_dest,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	return (0);
}


inline int
cmd_viewport(
	ID3D12GraphicsCommandList *cmdlist,
	FLOAT x, FLOAT y, FLOAT w, FLOAT h,
	FLOAT min_depth, FLOAT max_depth)
{
	D3D12_VIEWPORT viewport = {
		x, y, w, h, min_depth, max_depth
	};
	D3D12_RECT rect = {
		x, y, w, h
	};

	cmdlist->RSSetViewports(1, &viewport);
	cmdlist->RSSetScissorRects(1, &rect);

	return (0);
}

inline int
cmd_draw_instanced(
	ID3D12GraphicsCommandList *cmdlist, int vcnt, int inst)
{
	dbg("vcnt=%d\n", vcnt);
	cmdlist->DrawInstanced(vcnt, inst, 0, 0);
	return (0);
}

inline int
cmd_exec(
	ID3D12GraphicsCommandList *cmdlist,
	ID3D12CommandQueue *queue,
	ID3D12Fence *fence,
	uint64_t arg)
{
	ID3D12CommandList *pplists[] = {
		cmdlist,
	};

	cmdlist->Close();
	queue->ExecuteCommandLists(1, pplists);
	queue->Signal(fence, arg);
	return (0);
}

} //dx12cmd

