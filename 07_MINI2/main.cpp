#include "win.h"
#include "dx12.h"

#include <DirectXMath.h>

unsigned long random() {
	static unsigned long a = 1;
	static unsigned long b = 2345678;
	static unsigned long c = 9012345;
	a += b;
	b += c;
	c += a;
	return(a >> 16);
}

float frandom() {
	return float(random()) / float(0xFFFF);
}

float frandom2() {
	return frandom() * 2.0 - 1.0;
}

Device::Matrix GetTrans(float x, float y, float z) {
	using namespace DirectX;
	DirectX::XMMATRIX data = XMMatrixTranslation(x, y, z);
	return *(Device::Matrix *)&data;
}

Device::Matrix GetView(float px, float py, float pz,
                          float ax, float ay, float az,
                          float ux, float uy, float uz)
{
	using namespace DirectX;
	XMVECTOR vpos = XMVectorSet(px, py, pz, 1.0);
	XMVECTOR vat = XMVectorSet(ax, ay, az, 1.0);
	XMVECTOR vup = XMVectorSet(ux, uy, uz, 1.0);
	DirectX::XMMATRIX data = XMMatrixLookAtLH(vpos, vat, vup);
	return *(Device::Matrix *)&data;
}

Device::Matrix GetProj(float FovAngleY, float AspectRatio, float NearZ, float FarZ) {
	using namespace DirectX;
	DirectX::XMMATRIX data = XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);
	return *(Device::Matrix *)&data;
}

int main() {
	auto hWnd = win_init("minidx12", Device::Width, Device::Height);
	Device device(hWnd);

	//setup default param
	std::vector<Device::VertexFormat> vbuffer;
	float cube_vertex[] = {
		1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 
		1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 
		1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 
		1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 
		1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 
		1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 
		1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 
		1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 
		-1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 
		-1.0, -1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 
		1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 
		1.0, 1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 
	};

	for(int vcount = 0 ; vcount < _countof(cube_vertex); vcount += 3) {
		Device::VertexFormat data;
		data.position[0] = cube_vertex[vcount + 0];
		data.position[1] = cube_vertex[vcount + 1];
		data.position[2] = cube_vertex[vcount + 2];
		vbuffer.push_back(data);
	}
	device.UploadVertex(0, &vbuffer[0], vbuffer.size() * sizeof(Device::VertexFormat), sizeof(Device::VertexFormat));

	int present_count = 0;
	
	std::vector<Device::InstancingData> instdata(256);
	for(int i = 0 ; i < 256; i++) {
		Device::InstancingData *p = &instdata[i];
		p->world = GetTrans(frandom2() * 3.0, frandom2() * 3.0, frandom2() * 3.0);
		float scale = 0.1;
		p->world.data[0] = scale;
		p->world.data[5] = scale;
		p->world.data[10] = scale;
		p->color.data[0] = frandom();
		p->color.data[1] = frandom();
		p->color.data[2] = frandom();
		p->color.data[3] = 1.0;
	}
	std::vector<int> rendertarget_indices;
	for(int i = 0 ; i < 4; i++) {
		rendertarget_indices.push_back(Device::MAX_BACKBUFFER + i);
	}
	while(win_proc_msg()) {
		if(GetAsyncKeyState(VK_F5) & 0x0001) {
			device.ReloadPipeline();
		}

		Device::MatrixData matrixdata;
		
		matrixdata.view = GetView(
			cos(present_count * 0.01) * 3.0, 2, sin(present_count * 0.01) * 3.0,
			0, 0, 0,
			0, 1, 0);
		matrixdata.proj = GetProj(3.141592 / 2.0, float(Device::Width) / float(Device::Height), 0.1f, 256.0f);
		matrixdata.time.data[0] = float(present_count) * 1.0f / 60.0f;

		if(true)
		{
			int matrixdata_index = 0;
			int instancingdata_index = 1;
			device.UploadConstant(matrixdata_index, &matrixdata, sizeof(matrixdata));
			device.UploadConstant(instancingdata_index, &instdata[0], instdata.size() * sizeof(Device::InstancingData));

			float clear_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f, };
			device.Begin(&rendertarget_indices[0], rendertarget_indices.size(), rendertarget_indices[0], clear_color, true);
			if(device.SetEffect(1)) {
				device.SetVertex(0);
				device.SetConstant(0, matrixdata_index);
				device.SetConstant(1, instancingdata_index);
				device.commandlist->DrawInstanced(_countof(cube_vertex), instdata.size(), 0, 0);
			}
			device.End(rendertarget_indices.size());
		}

		{
			int matrixdata_index = 2;
			int instancingdata_index = 3;
			device.UploadConstant(matrixdata_index, &matrixdata, sizeof(matrixdata));
			device.UploadConstant(instancingdata_index, &instdata[0], instdata.size() * sizeof(Device::InstancingData));
            
			float clear_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f, };
			int rtvCount = 1;
			int swap_index = present_count % Device::MAX_BACKBUFFER;
			device.Begin(&swap_index, rtvCount, swap_index, clear_color);
			if(device.SetEffect(2)) {
				device.SetVertex(0);
				device.SetConstant(0, matrixdata_index);
				device.SetConstant(1, instancingdata_index);
				device.SetTexture(2, rendertarget_indices[0]);
				device.SetTexture(3, rendertarget_indices[1]);
				device.SetTexture(4, rendertarget_indices[2]);
				device.SetTexture(5, rendertarget_indices[3]);
				device.commandlist->DrawInstanced(_countof(cube_vertex), 1, 0, 0);
			}
			device.End(rtvCount);
		}
		device.Present();
		present_count++;
	}
	

	return 0;
}


