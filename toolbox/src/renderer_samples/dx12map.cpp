#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <windows.h>
#include <dwmapi.h>
#include <D3Dcompiler.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include "crc32.h"
#include "winapp.h"
#include "dx12cmd.h"

#include "node.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "D3DCompiler.lib")

#define MAX_INPUT_ELEMENT         (32)

using namespace dx12util;
using namespace dx12cmd;


struct dx12renderer
{
	////////////////////////////////////////////////////////////////////////
	//Frame用もろもろ
	////////////////////////////////////////////////////////////////////////
	struct frame_object
	{
		//コマンド周り
		ID3D12CommandAllocator * cmdallocator = nullptr;
		ID3D12GraphicsCommandList * cmdlist = nullptr;
		ID3D12Fence * fence = nullptr;
		uint64_t value;

		//backbufferはここに定義しておく
		ID3D12Resource * backbuffer = nullptr;

		//適当な名前。ああ、resから引っ張ってくるときに必要なんだっけ
		std::string name;
		std::map<std::string, ID3D12Resource *> mscratch;
		
		//mipmap生成が必要なrendertarget texture
		std::vector<std::string> vgenmipmap;

		//デバッグ用。いらねえかもし
		void print()
		{
			err("cmdallocator = %16p\n", cmdallocator);
		}
	};

	////////////////////////////////////////////////////////////////////////
	//Handle用定義
	////////////////////////////////////////////////////////////////////////
	struct handle_object
	{
		bool use = false;
		D3D12_CPU_DESCRIPTOR_HANDLE hcpu;
		D3D12_GPU_DESCRIPTOR_HANDLE hgpu;
	};

	//レンダラの使うやつら
	ID3D12Device * dev = nullptr;
	ID3D12CommandQueue * queue = nullptr;
	IDXGISwapChain3 * swap_chain = nullptr;

	std::vector<frame_object> frame_objects;

	//HeapとHandleプール
	std::map<int, ID3D12DescriptorHeap *> mheaps;
	std::map<int, std::vector<handle_object> > mhandlepool;

	//nodeデータ。あらゆるデータは全部これに乗る
	//レンダリングしたいデータをPerFrame作成するのではなく
	//DX12のデータもろもろ。めんどいね
	std::map<std::string, node *> mnode;
	std::map<std::string, handle_object> mhandles;
	std::map<std::string, ID3D12Resource *> mres;
	std::map<std::string, ID3D12RootSignature *> mroot_sigs;
	std::map<std::string, ID3D12PipelineState *> mpipeline_states;
	HANDLE hevent = nullptr;

	std::string get_backbuffer_name(int idx)
	{
		return "__backbuffer__" + std::to_string(idx);
	}

	std::string get_mip_name(std::string & name, int level)
	{
		return name + "_mip" + std::to_string(level);
	}

	std::string get_rtv_name(std::string & name)
	{
		return name + "_rtv";
	}

	std::string get_srv_name(std::string & name)
	{
		return name + "_srv";
	}

	std::string get_dsv_name(std::string & name)
	{
		return name + "_dsv";
	}
	
	std::string get_dummy_texture_name()
	{
		return "__DUMMY_TEX__";
	}

	void create_heap(D3D12_DESCRIPTOR_HEAP_TYPE type, int max_desc_size) {
		ID3D12DescriptorHeap * heap = nullptr;
		std::vector<handle_object> pool;

		create_desc_heap(dev, max_desc_size, type, &heap);
		auto inc_size = dev->GetDescriptorHandleIncrementSize(type);
		for (int i = 0 ; i < max_desc_size; i++)
		{
			auto hcpu = heap->GetCPUDescriptorHandleForHeapStart();
			auto hgpu = heap->GetGPUDescriptorHandleForHeapStart();
			hcpu.ptr += i * inc_size;
			hgpu.ptr += i * inc_size;
			pool.push_back({false, hcpu, hgpu});

		}
		mheaps[type] = heap;
		mhandlepool[type] = pool;
	}

	handle_object alloc_handle_object(std::vector<handle_object> & pool)
	{
		handle_object err;
		err.hcpu.ptr = 0x1234123412341234;
		err.hgpu.ptr = 0x7890789078907890;
		for (int i = 0 ; i < pool.size(); i++)
		{
			auto & h = pool[i];
			if (h.use == true) continue;
			pool[i].use = true;

			return h;
		}
		err("ERROR : no heap\n");
		return err;
	}

	handle_object alloc_handle_rtv(std::string name) {
		auto ret = alloc_handle_object(mhandlepool[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
		mhandles[name] = ret;
		return ret;
	}
	handle_object alloc_handle_dsv(std::string name) {
		auto ret = alloc_handle_object(mhandlepool[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]);
		mhandles[name] = ret;
		return ret;
	}
	handle_object alloc_handle_cbv(std::string name) {
		auto ret = alloc_handle_object(mhandlepool[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
		mhandles[name] = ret;
		return ret;
	}
	handle_object alloc_handle_srv(std::string name) {
		auto ret = alloc_handle_object(mhandlepool[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
		mhandles[name] = ret;
		return ret;
	}
	handle_object alloc_handle_uav(std::string name) {
		auto ret = alloc_handle_object(mhandlepool[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
		mhandles[name] = ret;
		return ret;
	}
	handle_object alloc_handle_sampler(std::string name) {
		auto ret = alloc_handle_object(mhandlepool[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]);
		mhandles[name] = ret;
		return ret;
	}

	void delete_shaders()
	{
		for (auto & x : mpipeline_states) {
			auto o = x.second;
			if (o) o->Release();
		}
		mpipeline_states.clear();
	}

	handle_object get_handle(std::string name) {
		if(mhandles.find(name) == mhandles.end()) {
			err("error name=%s\n", name.c_str());
			handle_object ret;
			return ret;
		}
		return mhandles[name];
	}
	void create_mipmap(ID3D12GraphicsCommandList *cmdlist, frame_object & ref)
	{
		auto mipunit = (unit *)mnode["mipmap"];
		dbg("mipunit = %p\n", mipunit);
		if(!mipunit) {
			err("Internal Error mipunit=NULL\n");
			return ;
		}
		auto root_sig = mroot_sigs["root"];
		auto pipeline_state = mpipeline_states[mipunit->get_shader_name()];

		auto heap_shader_res = mheaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
		std::vector<ID3D12DescriptorHeap *> heaplists;
		heaplists.push_back(heap_shader_res);
		cmdlist->SetGraphicsRootSignature(root_sig);
		cmdlist->SetDescriptorHeaps(heaplists.size(), heaplists.data());
		cmdlist->SetPipelineState(pipeline_state);
		auto & vgenmipmap = ref.vgenmipmap;
		for(auto & name : vgenmipmap) {
			auto n = mnode[name];
			auto res = mres[name];
			auto width  = 0;
			auto height = 0;
			if(n->get_type() == node::T_RENDERTARGET) {
				auto rt = (rendertarget *)n;
				width  = rt->get_width();
				height = rt->get_height();
			}
			if(n->get_type() == node::T_TEXTURE) {
				auto tex = (texture *)n;
				width  = tex->get_width();
				height = tex->get_height();
			}
			auto maxmiplevel = get_miplevels(width, height);
			for(int i = 0 ; i < maxmiplevel - 1; i++) {
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rt_cpu_handles;
				auto rstate_before = D3D12_RESOURCE_STATE_COMMON;
				auto rstate_after = D3D12_RESOURCE_STATE_RENDER_TARGET;
				auto src_name  = get_mip_name(get_srv_name(name), i + 0);
				auto dest_name = get_mip_name(get_rtv_name(name), i + 1);
				auto hsrc  = get_handle(src_name );
				auto hdest = get_handle(dest_name);
				rt_cpu_handles.push_back(hdest.hcpu);
				cmd_viewport(cmdlist, 0, 0, width >> 1, height >> 1, 0.0f, 1.0f);
				cmd_res_barrier(cmdlist, res, rstate_before, rstate_after);
				cmdlist->OMSetRenderTargets(rt_cpu_handles.size(), rt_cpu_handles.data(), FALSE, nullptr);
				cmdlist->SetGraphicsRootDescriptorTable(0, hsrc.hgpu);
				cmd_draw_instanced(cmdlist, 4, 1);
				cmd_res_barrier(cmdlist, res, rstate_after, rstate_before);
				width  >>= 1;
				height >>= 1;
			}
		}
		ref.vgenmipmap.clear();
	}

public:
	dx12renderer(HWND hwnd, int w, int h, int max_buffer,
	             int max_desc_size = 1024,
	             int root_sig_srv_num = 8,
	             int root_sig_cbv_num = 8,
	             int root_sig_uav_num = 8,
	             int root_sig_sampler_num = 2)
	{
		struct filter_data {
			std::string name;
			D3D12_FILTER filter;
		};
		const filter_data filters[] =
		{
			{"__FILTER_POINT__",   D3D12_FILTER_MIN_MAG_MIP_POINT},
			{"__FILTER_LINEAR__",  D3D12_FILTER_MIN_MAG_MIP_LINEAR},
		};

		frame_objects.resize(max_buffer);

		create_device(hwnd, &dev);
		create_cmdqueue(dev, &queue);
		create_swap_chain(queue, hwnd, w, h, max_buffer, &swap_chain);
		create_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, max_desc_size);
		create_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, max_desc_size);
		create_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, max_desc_size);
		create_heap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, max_desc_size);

		ID3D12RootSignature * root_sig = nullptr;
		create_root_sig(dev, &root_sig, root_sig_srv_num, root_sig_cbv_num, root_sig_uav_num, root_sig_sampler_num);
		mroot_sigs["root"] = root_sig;
		if (root_sig == nullptr) {
			dbg("root sigs = %p\n", root_sig);
			exit(0);
		}

		for (int i = 0; i < _countof(filters); i++) {
			auto ref = filters[i];
			auto & h_sampler = alloc_handle_sampler(ref.name);
			create_sampler(dev, ref.filter, h_sampler.hcpu);
		}

		for (int i = 0 ; i < max_buffer; i++) {
			auto & ref = frame_objects[i];
			create_cmdallocator(dev, &ref.cmdallocator);
			create_cmdlist(dev, ref.cmdallocator, &ref.cmdlist);
			create_fence(dev, &ref.fence);
			ref.value = -1;
			get_buffer_from_swapchain(swap_chain, i, &ref.backbuffer);

			std::string name = get_backbuffer_name(i);
			ref.name = name;
			mres[name] = ref.backbuffer;
			auto & h_rtv = alloc_handle_rtv(name);
			create_rtv(dev, ref.backbuffer, h_rtv.hcpu);
		}
		
		//ダミー用のテクスチャを作成する
		{
			std::vector<uint32_t> vdata;
			const int dw = 256;
			const int dh = 256;
			for (int h = 0; h < dh; h++) {
				for (int w = 0; w < dw; w++) {
					uint32_t value = (w >> 8) ^ (h >> 8);
					value |= value << 8;
					value |= value << 16;
					vdata.push_back(value);
				}
			}
			texture *dummy_tex = new texture(
				get_dummy_texture_name(), dw, dh, vdata.data(), vdata.size() * sizeof(uint32_t));
			set_node(dummy_tex->get_name(), dummy_tex);
		}
		
		//ミップマップ生成用のオブジェクトを作成する
		{
			auto u = new unit("mipmap");
			u->set_shader_name("genmipmap.hlsl");
			set_node(u->get_name(), u);
		}
		hevent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
	}

	void set_node(std::string name, node * n)
	{
		mnode[name] = n;
	}

	void update(uint64_t frame)
	{
		auto index = swap_chain->GetCurrentBackBufferIndex();
		auto & ref = frame_objects[index];
		auto & mscratch = ref.mscratch;
		auto & vgenmipmap = ref.vgenmipmap;
		
		//Frameに紐づいていた転送用のScratchバッファを開放する
		for (auto & x : mscratch)
		{
			auto r = x.second;
			r->Release();
		}
		mscratch.clear();
		
		//ミップももう作り終えてるはずなので殺す
		vgenmipmap.clear();

		auto root_sig = mroot_sigs["root"];
		for (auto np : mnode)
		{
			auto & n = np.second;
			auto name = n->get_name();
			auto type = n->get_type();
			auto update = n->get_update();
			n->mark_update(0);

			//まあ一時的なもの。数増えてきたらバカにならない処理時間になるはず
			auto name_srv = get_srv_name(name);
			auto name_dsv = get_dsv_name(name);
			auto res_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			//unitデータ
			if (type == node::T_UNIT)
			{
				auto u = (unit *)n;
				auto shader_name = u->get_shader_name();

				if (shader_name.empty())
					continue;

				//まず処理しようとしたUNITにbindされているshaderがあるかをチェックして
				//存在しなければ作成する。
				auto pso = mpipeline_states[shader_name];
				if (pso != nullptr)
					continue;

				D3D12_INPUT_ELEMENT_DESC iedesc[MAX_INPUT_ELEMENT];
				D3D12_GRAPHICS_PIPELINE_STATE_DESC pgdesc;
				std::vector<uint8_t> vscode;
				std::vector<uint8_t> pscode;

				auto name = shader_name.c_str();
				auto c_name = shader_name.c_str();

				memset(&pgdesc, 0, sizeof(pgdesc));
				memset(iedesc, 0, sizeof(iedesc));
				HRESULT ret = 0;
				ret |= compile_shader_from_file(c_name, "VSMain", "vs_5_0", vscode);
				ret |= compile_shader_from_file(c_name, "PSMain", "ps_5_0", pscode);
				if (ret)
				{
					err("%s : compile failed\n", c_name);
					continue;
				}

				//Debug //本当はnodeデータからattr取り出した方がいい？
				dbg("c_name=%s, vscode=%p, size=%zd\n", c_name, vscode.data(), vscode.size());
				dbg("c_name=%s, pscode=%p, size=%zd\n", c_name, pscode.data(), pscode.size());
				init_pipeline_state(&pgdesc);
				set_input_elements(iedesc, 0, "pos", 3, 0, FALSE);
				set_input_elements(iedesc, 1, "uv", 2, 0, FALSE);
				set_input_elements(iedesc, 2, "nor", 3, 0, FALSE);
				set_pipeline_input_element(&pgdesc, iedesc, 3);
				set_pipeline_vs_bytecode(&pgdesc, vscode.data(), vscode.size());
				set_pipeline_ps_bytecode(&pgdesc, pscode.data(), pscode.size());
				ID3D12PipelineState * temp = nullptr;
				ret |= create_pipeline_graphics_state(dev, &pgdesc, root_sig, &temp);
				if (ret)
				{
					err("Failed create_pipeline_graphics_state shader_name=%s\n", shader_name.c_str());
					continue;
				}
				mpipeline_states[shader_name] = temp;
			}

			//頂点データ
			if (type == node::T_VERTEX)
			{
				auto res = mres[name];
				auto vtx = (vertex *)n;
				ID3D12Resource * temp = nullptr;
				if (res != nullptr) continue;

				create_res(dev, vtx->get_size(), 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE, TRUE, &temp);
				//頂点データ作成してデータを転送しておく
				upload_data(temp, vtx->get_data(), vtx->get_size());
				mres[name] = temp;
			}

			//テクスチャデータを作成する
			if (type == node::T_TEXTURE)
			{
				auto tex = (texture *)n;
				auto res = mres[name];
				ID3D12Resource * temp = nullptr;
				auto fmt = DXGI_FORMAT_R8G8B8A8_UNORM; // temp
				
				//ミップマップ必要なら登録してoff
				if(tex->get_genmipmap()) {
					tex->set_genmipmap(false);
					vgenmipmap.push_back(name);
				}
				if (res != nullptr) continue;
				create_res(dev, tex->get_width(), tex->get_height(), fmt, res_flags, FALSE, &temp);

				//ハンドルを割り当ててしまう
				auto mipname_srv = get_mip_name(get_srv_name(name), 0);
				auto & h_srv = alloc_handle_srv(mipname_srv);
				create_srv(dev, temp, h_srv.hcpu);

				//スクラッチデータで転送予約しておく
				ID3D12Resource * scratch = nullptr;
				create_res(dev, tex->get_size(), 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE, TRUE, &scratch);
				upload_data(scratch, tex->get_data(), tex->get_size());
				mscratch[name] = scratch;
				
				mres[name] = temp;
			}

			//レンダーターゲット
			if (type == node::T_RENDERTARGET)
			{
				auto rt = (rendertarget *)n;
				if(rt->get_genmipmap()) {
					rt->set_genmipmap(false);
					vgenmipmap.push_back(name);
				}
				auto res = mres[name];
				if (res != nullptr) continue;
				auto fmt = DXGI_FORMAT_R8G8B8A8_UNORM; // temp
				auto dfmt = DXGI_FORMAT_D32_FLOAT;
				ID3D12Resource * temp = nullptr;
				ID3D12Resource * temp_depth = nullptr;

				//RT, DSを作成しておく
				create_res(dev, rt->get_width(), rt->get_height(), fmt, res_flags, FALSE, &temp);
				create_res(dev, rt->get_width(), rt->get_height(), dfmt, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, FALSE, &temp_depth);

				//ミップマップ用のハンドルも参照してしまう～がもしかして中間バッファ作ってdraw -> copy、中間バッファ破棄で良いのかな
				int max_mip = get_miplevels(rt->get_width(), rt->get_height());
				for(int i = 0 ; i < max_mip; i++) {
					auto mipname_rtv = get_mip_name(get_rtv_name(name), i);
					auto & h_rtv = alloc_handle_rtv(mipname_rtv);
					create_rtv(dev, temp, h_rtv.hcpu, i);

					auto mipname_srv = get_mip_name(get_srv_name(name), i);
					auto & h_srv = alloc_handle_srv(mipname_srv);
					create_srv(dev, temp, h_srv.hcpu, i);
				}

				auto & h_dsv = alloc_handle_dsv(get_dsv_name(name));
				create_dsv(dev, temp_depth, h_dsv.hcpu);

				mres[name] = temp;
				mres[name_dsv] = temp_depth;
			}
		}
	}
	

	void draw(uint64_t frame)
	{
		dbg("============================================================\n");
		dbg(" DRAW START\n");
		dbg("============================================================\n");
		std::vector<view *> viewlists;

		auto index = swap_chain->GetCurrentBackBufferIndex();
		auto & ref = frame_objects[index];
		auto root_sig = mroot_sigs["root"];
		auto heap_shader_res = mheaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
		auto heap_sampler = mheaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];

		//Viewの整理整頓をする
		for (auto np : mnode)
		{
			auto & n = np.second;
			auto type = n->get_type();
			if (type == node::T_VIEW)
			{
				view * v = (view *)n;
				viewlists.push_back(v);
			}
		}
		std::string backbuffer_name = get_backbuffer_name(index);
		dbg("backbuffer_name=%s\n", backbuffer_name.c_str());
		
		dbg("viewlists num=%d\n", viewlists.size());
		std::sort(
			viewlists.begin(),
			viewlists.end(),
			[](view *a, view *b) {
				dbg("sort\n");
				return a->get_order() > b->get_order();
			}
		);

		//ここでコマンドバッファ全部処理したかをチェックして必要だったらちゃんと待つよ うにする
		auto & fence = ref.fence;
		auto & value = ref.value;
		auto cvalue = fence->GetCompletedValue();
		if (value != -1 && cvalue != value)
		{
			fence->SetEventOnCompletion(value, hevent);
			WaitForSingleObject(hevent, INFINITE);
		}

		std::vector<ID3D12DescriptorHeap *> heaplists;
		heaplists.push_back(heap_shader_res);

		dbg("RESET CMD frame=%d\n", frame);
		//////////////////////////////////////////////////////////////////////////////////////////
		// RESET CMD // ここから楽しいコマンド積みの開始だぞ
		//////////////////////////////////////////////////////////////////////////////////////////
		auto & cmdlist = ref.cmdlist;
		auto & cmdallocator = ref.cmdallocator;
		
		//りせっと
		cmd_reset(cmdlist, cmdallocator);

		//ここでスクラッチデータの転送予約を行ってレンダリングの準備をする。同期処理と か必要だったらおいおい行うこと...
		auto & mscratch = ref.mscratch;
		for (auto & x : mscratch)
		{
			auto scratch_name = x.first;
			auto dest = mres[scratch_name];
			auto src = x.second;
			if (!dest || !src) {
				err("cmd_res_copy failed dest=%p, src=%p\n", dest, src);
				continue;
			}
			
			cmd_res_copy(dev, cmdlist, dest, src, 0);
			dbg("cmd_res_copy scratch_name=%s, dest=%p, src=%p\n", scratch_name.c_str(), dest, src);
		}
		

		//Graphics周りBindRes設定
		cmdlist->SetGraphicsRootSignature(root_sig);
		cmdlist->SetDescriptorHeaps(heaplists.size(), heaplists.data());

		//Viewごとに処理を積む。とあるViewが処理終わったらそれを使いますよ～とかもここ でViewをうまくつかってやってつかあさい...
		for (auto & vi : viewlists)
		{
			dbg("View name = %s\n", vi->get_name().c_str());
			auto rt = vi->get_rendertarget();
			auto rtvhandle = mhandles[backbuffer_name];
			auto rstate_before = D3D12_RESOURCE_STATE_PRESENT;
			auto rstate_after = D3D12_RESOURCE_STATE_RENDER_TARGET;
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rt_cpu_handles;
			std::vector<ID3D12Resource *> rt_res;
			float ccolor[4] = {};

			//クリアカラーを取得する
			vi->get_clearcolor(ccolor);
			dbg("clear_color(%f %f %f %f)\n", ccolor[0], ccolor[1], ccolor[2], ccolor[3]);

			/* Setup framebuffer */
			cmd_viewport(cmdlist, 0, 0, vi->get_width(), vi->get_height(), 0.0f, 1.0f);

			//Viewに紐づいてるRenderTargetがあるならRTVHandleを取得すること
			if (rt)
			{
				rstate_before = D3D12_RESOURCE_STATE_COMMON;
				rstate_after = D3D12_RESOURCE_STATE_RENDER_TARGET;
				auto name = rt->get_name();
				auto name_rtv = get_mip_name(get_rtv_name(name), 0);
				dbg("Using Render Target : OMSetRenderTargets[%d]=%s\n", 0, name.c_str());
				dbg("Using Render Target : OMSetRenderTargets RTV[%d]=%s\n", 0, name_rtv.c_str());
				auto res = mres[name];
				rtvhandle = get_handle(name_rtv);
				rt_cpu_handles.push_back(rtvhandle.hcpu);
				rt_res.push_back(res);
			}
			else
			{
				dbg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				dbg("!!!!!!! Using Swap Buffer RT\n");
				dbg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				auto res = mres[backbuffer_name];
				rt_cpu_handles.push_back(rtvhandle.hcpu);
				rt_res.push_back(res);
			}

			//バリア発酵
			for (auto & res : rt_res)
			{
				cmd_res_barrier(cmdlist, res, rstate_before, rstate_after);
			}

			//RTセットアップ
			for(int i = 0 ; i < rt_cpu_handles.size(); i++ ) {
				cmdlist->OMSetRenderTargets(rt_cpu_handles.size(), rt_cpu_handles.data(), FALSE, nullptr);
			}

			//クリア
			for (auto & h : rt_cpu_handles) {
				cmdlist->ClearRenderTargetView(h, ccolor, 0, NULL);
			}

			//Viewに登録されているunitsごとにコマンドを積む
			auto units =  vi->get_units();
			for (auto & x : units)
			{
				auto u = x.second;
				auto vertex_name = u->get_vertex_name();
				auto texture_name = u->get_texture_name();
				auto shader_name = u->get_shader_name();
				std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> gpuhandles_shader_res;

				//頂点バッファ指定
				if (vertex_name.empty()) {
					err("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					err("Error Empty vertex unit=%s\n", u->get_name().c_str());
					err("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					continue;
				}

				auto vtx = (vertex *)mnode[vertex_name];
				if(!vtx) {
					err("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					err("Error Empty vertex unit=%s\n", u->get_name().c_str());
					err("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					continue;
				}
				auto vertex_res = mres[vertex_name];
				
				D3D12_VERTEX_BUFFER_VIEW view = {};
				view.BufferLocation = vertex_res->GetGPUVirtualAddress();
				view.SizeInBytes = vtx->get_size();
				view.StrideInBytes = vtx->get_stride_size();
				cmdlist->IASetVertexBuffers(0, 1, &view);
				cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				//todo u->get_topology();
				
				//必ずシェーダーが指定されているはず。指定されていなかったら絵が描けないので無視する
				auto pipeline_state = mpipeline_states[shader_name];
				if (shader_name.empty() || !pipeline_state) {
					err("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					err("Error shader unit=%s\n", u->get_name().c_str());
					err("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					continue;
				}
				dbg("shader name=%s\n", shader_name.c_str());
				cmdlist->SetPipelineState(pipeline_state);

				//テクスチャ紐づけ
				//テクスチャあったらgpu側にpushしておく。ほんらいはindex気にしたいところだね
				if (!texture_name.empty()) {
					auto tex = mnode[texture_name];
					err("texture_name=%s\n", texture_name.c_str());
					if (tex) {
						auto name_srv = get_mip_name(get_srv_name(texture_name), 0);
						if (mhandles.find(name_srv) != mhandles.end()) {
							auto & h = mhandles[name_srv];
							gpuhandles_shader_res.push_back(h.hgpu);
						} else {
							printf("ERROR tex = nullptr\n");
							tex = nullptr;
						}
					}
					if(!tex) {
						auto dummy_tex_name = get_dummy_texture_name();
						auto name_srv = get_srv_name(dummy_tex_name);
						auto & h = mhandles[name_srv];
						gpuhandles_shader_res.push_back(h.hgpu);
					}
				}

				for(int i = 0; i < gpuhandles_shader_res.size(); i++)
					cmdlist->SetGraphicsRootDescriptorTable(i, gpuhandles_shader_res[i]);
				cmd_draw_instanced(cmdlist, u->get_vertex_num(), 1);
			}

			//アンバリア発行。本当は依存関係が出た時だけ発行すればいいはず...
			for (auto & res : rt_res)
				cmd_res_barrier(cmdlist, res, rstate_after, rstate_before);

			//そしてmipmap作成
			create_mipmap(cmdlist, ref);
		}

		cmd_exec(cmdlist, queue, fence, frame);
		ref.value = frame;

		//フリップ
		swap_chain->Present(1, 0);

		dbg("============================================================\n");
		dbg(" DRAW END\n");
		dbg("============================================================\n\n");
	}
};

#define WIDTH                     (720)
#define HEIGHT                    (480)
#define MAX_BUFFER                (2)
int
main()
{
	using namespace dx12util;
	using namespace dx12cmd;
	HWND hwnd = nullptr;

	//windows
	winapp app;
	app.create("test", WIDTH, HEIGHT);
	hwnd = app.get_handle();
	
	//dx12
	dx12renderer renderer(hwnd, WIDTH, HEIGHT, MAX_BUFFER);

	//頂点データ作成
	struct vertex_format {
		float pos[3];
		float uv[2];
		float nor[3];
	};
	std::vector<vertex_format> rect_data = {
		{-1, -1, 0, 0, 0, 1, 0, 0},
		{-1,  1, 0, 0, 1, 1, 0, 0},
		{ 1, -1, 0, 1, 0, 1, 0, 0},
		{ 1,  1, 0, 1, 1, 1, 0, 0},
	};

	//雑にテクスチャを作成する
	texture *test_tex = nullptr;
	{
		std::vector<uint32_t> vdata;
		for (int h = 0; h < 256; h++) {
			for (int w = 0; w < 256; w++) {
				vdata.push_back(w ^ h + (h << 8));
			}
		}
		test_tex = new texture("testtex", 256, 256, vdata.data(), vdata.size() * 4);
		renderer.set_node(test_tex->get_name(), test_tex);
	}

	//頂点データ作成
	auto rect_vertex = new vertex("vertex_rect", rect_data.data(), rect_data.size() * sizeof(vertex_format), sizeof(vertex_format));
	auto test_rt = new rendertarget("testrt", WIDTH, HEIGHT);

	//RT
	auto u_rect = new unit("rect");
	u_rect->set_shader_name("rect.hlsl");
	u_rect->set_texture_name(test_tex->get_name());
	u_rect->set_vertex_name(rect_vertex->get_name());
	u_rect->set_vertex_num(rect_data.size());

	auto test_view = new view("testview", WIDTH, HEIGHT);
	test_view->set_rendertarget(test_rt);
	test_view->set_clearcolor(1, 1, 0, 1);
	test_view->set_order(100);
	test_view->set_unit(u_rect->get_name(), u_rect);

	//PRESENT
	auto u_present = new unit("present_rect");
	u_present->set_shader_name("present.hlsl");
	u_present->set_texture_name(test_rt->get_name());
	u_present->set_vertex_name(rect_vertex->get_name());
	u_present->set_vertex_num(rect_data.size());

	auto present_view = new view("presentview", WIDTH, HEIGHT);
	present_view->set_order(0);
	present_view->set_clearcolor(1, 0, 0, 1);
	present_view->set_unit(u_present->get_name(), u_present);
	present_view->set_rendertarget(nullptr);

	renderer.set_node(test_view->get_name(), test_view);
	renderer.set_node(present_view->get_name(), present_view);
	renderer.set_node(test_rt->get_name(), test_rt);
	renderer.set_node(u_present->get_name(), u_present);
	renderer.set_node(u_rect->get_name(), u_rect);
	renderer.set_node(rect_vertex->get_name(), rect_vertex);
	
	//必須なnodeだけ登録しておく
	for (uint64_t frame = 0; app.update(); frame++)
	{
		present_view->set_clearcolor(1, frame & 1, 0, 1);
		if ( (GetAsyncKeyState(VK_F5) & 0x0001) )
			renderer.delete_shaders();
		test_rt->set_genmipmap(true);
		renderer.update(frame);
		renderer.draw(frame);
	}
	return 0;
}
