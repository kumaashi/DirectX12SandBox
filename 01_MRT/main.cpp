#include "common.h"
#include "Buffer.h"
#include "Dx12.h"
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
	return float(random()) / float(0x7FFF);
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

VertexBuffer *CreateRectBuffer(VertexBuffer * ret)
{
	ret->Create(6);
	ret->SetPos(0, -1, 1, 0);
	ret->SetPos(1, 1, 1, 0);
	ret->SetPos(2, -1, -1, 0);
	ret->SetPos(3, 1, 1, 0);
	ret->SetPos(4, 1, -1, 0);
	ret->SetPos(5, -1, -1, 0);
	ret->MarkUpdate();
	return ret;
}

VertexBuffer *CreateCubeBuffer(VertexBuffer * ret)
{
	ret->Create(6 * 6);
	int i = 0;
	ret->SetPos(i++, 1.0, -1.0, -1.0);
	ret->SetPos(i++, 1.0, -1.0, 1.0);
	ret->SetPos(i++, -1.0, -1.0, 1.0);
	ret->SetPos(i++, 1.0, -1.0, -1.0);
	ret->SetPos(i++, -1.0, -1.0, 1.0);
	ret->SetPos(i++, -1.0, -1.0, -1.0);
	ret->SetPos(i++, 1.0, 1.0, -1.0);
	ret->SetPos(i++, -1.0, 1.0, -1.0);
	ret->SetPos(i++, -1.0, 1.0, 1.0);
	ret->SetPos(i++, 1.0, 1.0, -1.0);
	ret->SetPos(i++, -1.0, 1.0, 1.0);
	ret->SetPos(i++, 1.0, 1.0, 1.0);
	ret->SetPos(i++, 1.0, -1.0, -1.0);
	ret->SetPos(i++, 1.0, 1.0, -1.0);
	ret->SetPos(i++, 1.0, 1.0, 1.0);
	ret->SetPos(i++, 1.0, -1.0, -1.0);
	ret->SetPos(i++, 1.0, 1.0, 1.0);
	ret->SetPos(i++, 1.0, -1.0, 1.0);
	ret->SetPos(i++, 1.0, -1.0, 1.0);
	ret->SetPos(i++, 1.0, 1.0, 1.0);
	ret->SetPos(i++, -1.0, 1.0, 1.0);
	ret->SetPos(i++, 1.0, -1.0, 1.0);
	ret->SetPos(i++, -1.0, 1.0, 1.0);
	ret->SetPos(i++, -1.0, -1.0, 1.0);
	ret->SetPos(i++, -1.0, -1.0, 1.0);
	ret->SetPos(i++, -1.0, 1.0, 1.0);
	ret->SetPos(i++, -1.0, 1.0, -1.0);
	ret->SetPos(i++, -1.0, -1.0, 1.0);
	ret->SetPos(i++, -1.0, 1.0, -1.0);
	ret->SetPos(i++, -1.0, -1.0, -1.0);
	ret->SetPos(i++, 1.0, 1.0, -1.0);
	ret->SetPos(i++, 1.0, -1.0, -1.0);
	ret->SetPos(i++, -1.0, -1.0, -1.0);
	ret->SetPos(i++, 1.0, 1.0, -1.0);
	ret->SetPos(i++, -1.0, -1.0, -1.0);
	ret->SetPos(i++, -1.0, 1.0, -1.0);
	ret->MarkUpdate();
	return ret;
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
	TextureBuffer *tex0 = renderer->GetTexture("tex0");
	{
		enum
		{
			Width = 256,
			Height = 256,
		};
		tex0->Create(Width, Height, 0);
		std::vector < unsigned char >vbuffer;
		for(int j = 0; j < Height; j++)
		{
			for(int i = 0; i < Width; i++)
			{
				//RGBA8
				vbuffer.push_back(i ^ j * 1);
				vbuffer.push_back(i ^ j * 2);
				vbuffer.push_back(i ^ j * 3);
				vbuffer.push_back(i ^ j * 4);
			}
		}
		tex0->SetData(&vbuffer[0], vbuffer.size());
	}

	VertexBuffer *geo0 = CreateRectBuffer(renderer->GetGeometry("geo0"));
	VertexBuffer *geo1 = CreateRectBuffer(renderer->GetGeometry("geo1"));
	VertexBuffer *base = CreateCubeBuffer(renderer->GetGeometry("cube"));
	ViewBuffer *view0 = renderer->GetViewPort("view0");
	ViewBuffer *view1 = renderer->GetViewPort("view1");
	RenderTargetBuffer *rt0 = renderer->GetRenderTarget("rt0");
	RenderTargetBuffer *rt1 = renderer->GetRenderTarget("rt1");
	rt0->Create(Width, Height, 0, RenderTargetBuffer::TypeTargetBuffer, 4);

	view0->SetOrder(0);
	view0->SetClearColor(1, 1, 1, 1);
	view0->SetViewPort(0, 0, Width, Height);
	view0->SetRenderTarget(rt0);
	base->SetShaderName("normal.hlsl");
	view0->SetVertexBuffer(base->GetName(), base);

	view1->SetOrder(0);
	view1->SetClearColor(1, 1, 1, 1);
	view1->SetViewPort(0, 0, Width, Height);
	view1->SetVertexBuffer(geo1->GetName(), geo1);
	view1->SetRenderTarget(0);

	geo1->SetShaderName("present.hlsl");
	geo1->SetTextureIndex(0, rt0->GetTexture(0));
	geo1->SetTextureIndex(1, rt0->GetTexture(1));
	geo1->SetTextureIndex(2, rt0->GetTexture(2));
	geo1->SetTextureIndex(3, rt0->GetTexture(3));
	geo1->SetTextureIndex(4, tex0);

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
		float UpY = 1;
		float UpZ = 0;

		XMMATRIX xm_view = GetView(PosX, PosY, PosZ,
		                            AtX, AtY, AtZ,
		                            UpX, UpY, UpZ);
		MatrixData proj = *(MatrixData *)(&xm_proj);
		MatrixData view = *(MatrixData *)(&xm_view);
		view0->SetProjMatrix(proj);
		view0->SetViewMatrix(view);
		view1->SetProjMatrix(proj);
		view1->SetViewMatrix(view);

		renderer->Update();
		renderer->Render("view0");
		renderer->Render("view1");
		renderer->Present();
		ShowFps();
	}

	delete renderer;
}
