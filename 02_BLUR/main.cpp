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
	i = 0;
	ret->SetNormal(i++,0.0, -1.0, 0.0);
	ret->SetNormal(i++,0.0, -1.0, 0.0);
	ret->SetNormal(i++,0.0, -1.0, 0.0);
	ret->SetNormal(i++,0.0, -1.0, 0.0);
	ret->SetNormal(i++,0.0, -1.0, 0.0);
	ret->SetNormal(i++,0.0, -1.0, 0.0);
	ret->SetNormal(i++,0.0, 1.0, 0.0);
	ret->SetNormal(i++,0.0, 1.0, 0.0);
	ret->SetNormal(i++,0.0, 1.0, 0.0);
	ret->SetNormal(i++,0.0, 1.0, 0.0);
	ret->SetNormal(i++,0.0, 1.0, 0.0);
	ret->SetNormal(i++,0.0, 1.0, 0.0);
	ret->SetNormal(i++,1.0, 0.0, 0.0);
	ret->SetNormal(i++,1.0, 0.0, 0.0);
	ret->SetNormal(i++,1.0, 0.0, 0.0);
	ret->SetNormal(i++,1.0, 0.0, 0.0);
	ret->SetNormal(i++,1.0, 0.0, 0.0);
	ret->SetNormal(i++,1.0, 0.0, 0.0);
	ret->SetNormal(i++,-0.0, -0.0, 1.0);
	ret->SetNormal(i++,-0.0, -0.0, 1.0);
	ret->SetNormal(i++,-0.0, -0.0, 1.0);
	ret->SetNormal(i++,-0.0, -0.0, 1.0);
	ret->SetNormal(i++,-0.0, -0.0, 1.0);
	ret->SetNormal(i++,-0.0, -0.0, 1.0);
	ret->SetNormal(i++,-1.0, -0.0, -0.0);
	ret->SetNormal(i++,-1.0, -0.0, -0.0);
	ret->SetNormal(i++,-1.0, -0.0, -0.0);
	ret->SetNormal(i++,-1.0, -0.0, -0.0);
	ret->SetNormal(i++,-1.0, -0.0, -0.0);
	ret->SetNormal(i++,-1.0, -0.0, -0.0);
	ret->SetNormal(i++,0.0, 0.0, -1.0);
	ret->SetNormal(i++,0.0, 0.0, -1.0);
	ret->SetNormal(i++,0.0, 0.0, -1.0);
	ret->SetNormal(i++,0.0, 0.0, -1.0);
	ret->SetNormal(i++,0.0, 0.0, -1.0);
	ret->SetNormal(i++,0.0, 0.0, -1.0);
	
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

	VertexBuffer *base = CreateCubeBuffer(renderer->GetGeometry("cube"));
	VertexBuffer *geo_blur_x         = CreateRectBuffer(renderer->GetGeometry("geo_blur_x"));
	VertexBuffer *geo_blur_y         = CreateRectBuffer(renderer->GetGeometry("geo_blur_y"));
	VertexBuffer *geo_present        = CreateRectBuffer(renderer->GetGeometry("geo_present"));
	ViewBuffer *view_gbuffer         = renderer->GetViewPort("view_gbuffer");
	ViewBuffer *view_blur_x          = renderer->GetViewPort("view_blur_x");
	ViewBuffer *view_blur_y          = renderer->GetViewPort("view_blur_y");
	ViewBuffer *view_present         = renderer->GetViewPort("view_present");
	RenderTargetBuffer *rt_gbuffer   = renderer->GetRenderTarget("rt_gbuffer");
	RenderTargetBuffer *rt_blur_x    = renderer->GetRenderTarget("rt_blur_x");
	RenderTargetBuffer *rt_blur_y    = renderer->GetRenderTarget("rt_blur_y");
	rt_gbuffer->Create(Width, Height, 0, RenderTargetBuffer::TypeTargetBuffer, 4);
	rt_blur_x->Create(Width, Height, 0, RenderTargetBuffer::TypeTargetBuffer, 4);
	rt_blur_y->Create(Width, Height, 0, RenderTargetBuffer::TypeTargetBuffer, 4);

	base->SetShaderName("normal.hlsl");

	geo_blur_x->SetShaderName("blur.hlsl");
	geo_blur_y->SetShaderName("blur.hlsl");
	geo_present->SetShaderName("present.hlsl");

	//set view
	view_gbuffer->SetOrder(0);
	view_gbuffer->SetClearColor(0, 0, 0, 0);
	view_gbuffer->SetViewPort(0, 0, Width, Height);
	view_gbuffer->SetRenderTarget(rt_gbuffer);
	view_gbuffer->SetVertexBuffer(base->GetName(), base);

	view_blur_x->SetOrder(0);
	view_blur_x->SetEffectParam(0, 0, 0, 0);
	view_blur_x->SetClearColor(0, 0, 0, 0);
	view_blur_x->SetViewPort(0, 0, Width, Height);
	view_blur_x->SetVertexBuffer(geo_blur_x->GetName(), geo_blur_x);
	view_blur_x->SetRenderTarget(rt_blur_x);
	
	view_blur_y->SetOrder(0);
	view_blur_y->SetEffectParam(1, 0, 0, 0);
	view_blur_y->SetClearColor(0, 0, 0, 0);
	view_blur_y->SetViewPort(0, 0, Width, Height);
	view_blur_y->SetVertexBuffer(geo_blur_y->GetName(), geo_blur_y);
	view_blur_y->SetRenderTarget(rt_blur_y);
	
	view_present->SetOrder(0);
	view_present->SetClearColor(0, 0, 0, 0);
	view_present->SetViewPort(0, 0, Width, Height);
	view_present->SetVertexBuffer(geo_present->GetName(), geo_present);
	view_present->SetRenderTarget(0);

	geo_blur_x->SetShaderName("blur.hlsl");
	geo_blur_x->SetTextureIndex(0, rt_gbuffer->GetTexture(0));
	geo_blur_x->SetTextureIndex(1, rt_gbuffer->GetTexture(1));
	geo_blur_x->SetTextureIndex(2, rt_gbuffer->GetTexture(2));
	geo_blur_x->SetTextureIndex(3, rt_gbuffer->GetTexture(3));
	geo_blur_x->SetTextureIndex(4, rt_gbuffer->GetTexture(0));

	geo_blur_y->SetShaderName("blur.hlsl");
	geo_blur_y->SetTextureIndex(0, rt_gbuffer->GetTexture(0));
	geo_blur_y->SetTextureIndex(1, rt_gbuffer->GetTexture(0));
	geo_blur_y->SetTextureIndex(2, rt_gbuffer->GetTexture(0));
	geo_blur_y->SetTextureIndex(3, rt_gbuffer->GetTexture(0));
	geo_blur_y->SetTextureIndex(4, rt_blur_x->GetTexture(0));
	
	geo_present->SetShaderName("present.hlsl");
	geo_present->SetTextureIndex(0, rt_gbuffer->GetTexture(0));
	geo_present->SetTextureIndex(1, rt_gbuffer->GetTexture(1));
	geo_present->SetTextureIndex(2, rt_gbuffer->GetTexture(2));
	geo_present->SetTextureIndex(3, rt_gbuffer->GetTexture(3));
	geo_present->SetTextureIndex(4, rt_blur_y->GetTexture(0));

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
		
		view_gbuffer->SetViewMatrix(view);
		view_blur_x ->SetViewMatrix(view);
		view_blur_y ->SetViewMatrix(view);
		view_present->SetViewMatrix(view);
		
		view_gbuffer->SetProjMatrix(proj);
		view_blur_x ->SetProjMatrix(proj);
		view_blur_y ->SetProjMatrix(proj);
		view_present->SetProjMatrix(proj);
		
		

		renderer->Update();
		renderer->Render("view_gbuffer");
		renderer->Render("view_blur_x");
		renderer->Render("view_blur_y");
		renderer->Render("view_present");
		renderer->Present();
		ShowFps();
	}

	delete renderer;
}
