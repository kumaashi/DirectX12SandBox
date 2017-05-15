
#pragma  warning(disable:4996)
#pragma  warning(disable:4313)
#pragma  warning(disable:4477)


#include "Dx12.h"
#include "common.h"
#include "Win.h"

unsigned long random()
{
	static unsigned long a = 1;
	static unsigned long b = 2345678;
	static unsigned long c = 9012345;
	a += b;
	b += c;
	c += a;
	return(a >> 16);
}

float frandom()
{
	return float(random()) / float(0xFFFF);
}

float frandom2()
{
	return frandom() * 2.0 - 1.0;
}


int ShowFps()
{
	static DWORD last = timeGetTime();
	static DWORD frames = 0;
	static char buf[256] = "";
	DWORD current;
	current = timeGetTime();
	frames++;
	if(1000 <= current - last)
	{
		float dt =(float)(current - last) / 1000.0f;
		float fps =(float) frames / dt;
		last = current;
		frames = 0;
		sprintf(buf, "%.02f fps", fps);
		printf("%s\n", buf);
	}
	return 0;
}


DirectX::XMMATRIX GetTrans(float x, float y, float z) {
	using namespace DirectX;
	return XMMatrixTranslation(x, y, z);
}

DirectX::XMMATRIX GetView(float px, float py, float pz,
                          float ax, float ay, float az,
                          float ux, float uy, float uz)
{
	using namespace DirectX;
	XMVECTOR vpos = XMVectorSet(px, py, pz, 1.0);
	XMVECTOR vat = XMVectorSet(ax, ay, az, 1.0);
	XMVECTOR vup = XMVectorSet(ux, uy, uz, 1.0);
	return XMMatrixLookAtLH(vpos, vat, vup);
}

DirectX::XMMATRIX GetProj(float FovAngleY,
                          float AspectRatio, float NearZ, float FarZ)
{
	using namespace DirectX;
	return XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);
}


int main() {
	using namespace Dx12;
	using namespace DirectX;
	enum
	{
		Width = 1280,
		Height = 720,
		FrameCount = 2,
	};


	HWND hWnd = win_init("DirectX12 test update", Width, Height);
	Renderer *renderer = new Renderer(hWnd, Width, Height, FrameCount);
	VertexBuffer *base               = CreateCubeBuffer(renderer->GetGeometry("cube"));
	VertexBuffer *rect               = CreateCubeBuffer(renderer->GetGeometry("rect"));
	InstancingBuffer *inst           = renderer->GetInstancing("instance");
	ViewBuffer *view_present         = renderer->GetViewPort("view_present");
	base->SetShaderName("normal.hlsl");
	rect->SetShaderName("normal.hlsl");
	view_present->SetOrder(0);
	view_present->SetClearColor(0, 1, 1, 0);
	view_present->SetViewPort(0, 0, Width, Height);
	view_present->SetInstancingBuffer(inst->GetName(), inst);
	view_present->SetRenderTarget(0);

	//set instance vertex
	inst->SetVertexBuffer(base->GetName(), base);
	inst->SetVertexBuffer(rect->GetName(), rect);

	InstancingBuffer::Data data;
	for(int i = 0 ; i < 4096; i++) {
		XMMATRIX xm_world = GetTrans(frandom2() * 3.0, frandom2() * 3.0, frandom2() * 3.0);
		data.world = *(MatrixData *)(&xm_world);
		//0   1  2  3
		//4   5  6  7
		//8   9 10 11
		//12 13 14 15
		float scale = 0.1;
		data.world.data[0] = scale;
		data.world.data[5] = scale;
		data.world.data[10] = scale;
		data.color.data[0] = frandom();
		data.color.data[1] = frandom();
		data.color.data[2] = frandom();
		data.color.data[3] = 1.0;
		inst->AddData(data);
	}
	while(win_proc_msg())
	{
		if(GetAsyncKeyState(VK_F5) & 0x0001)
		{
			renderer->DeleteShader();
		}
		XMMATRIX xm_proj =
		    GetProj(3.141592 / 2.0, float(Width) / float(Height), 0.1, 1000.0);
		static float a = 0.0;
		a += 0.01;

		float PosX = sin(a) * 5;
		float PosY = 3.0f;
		float PosZ = cos(a) * 5;

		float AtX = 0;
		float AtY = 0;
		float AtZ = 0;

		float UpX = 0;
		float UpY = 2;
		float UpZ = 0;

		XMMATRIX xm_view = GetView(PosX, PosY, PosZ,
		                           AtX, AtY, AtZ,
		                           UpX, UpY, UpZ);
		MatrixData proj = *(MatrixData *)(&xm_proj);
		MatrixData view = *(MatrixData *)(&xm_view);

		view_present->SetViewMatrix(view);
		view_present->SetProjMatrix(proj);

		renderer->Update();
		renderer->Render("view_present");
		renderer->Present();
		ShowFps();
	}

	delete renderer;
}
