#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>

#pragma  comment(lib, "gdi32.lib")
#pragma  comment(lib, "winmm.lib")
#pragma  comment(lib, "user32.lib")
#pragma  comment(lib, "glu32.lib")
#pragma  comment(lib, "dxgi.lib")
#pragma  comment(lib, "d3d12.lib")
#pragma  comment(lib, "D3DCompiler.lib")

#define RELEASE(x) {if(x) {x->Release(); x = NULL; } }
typedef std::vector<unsigned char> D3DShaderVector;

struct File {
	std::vector<unsigned char> buf;
	size_t Size() {
		return buf.size();
	}

	File(const char *name, const char *mode = "rb") {
		FILE *fp = fopen(name, mode);
		if (fp) {
			fseek(fp, 0, SEEK_END);
			buf.resize(ftell(fp));
			fseek(fp, 0, SEEK_SET);
			fread(&buf[0], 1, buf.size(), fp);
			fclose(fp);
		} else {
			printf("Cant Open File -> %s\n", name);
		}
	}

	void *Buf() {
		return (void *)(&buf[0]);
	}
};




inline VertexBuffer *CreateRectBuffer(VertexBuffer * ret)
{
	ret->Create(6);
	float scale = 2.0f;
	ret->SetPos(0, -scale,  scale, 0);
	ret->SetPos(1,  scale,  scale, 0);
	ret->SetPos(2, -scale, -scale, 0);
	ret->SetPos(3,  scale,  scale, 0);
	ret->SetPos(4,  scale, -scale, 0);
	ret->SetPos(5, -scale, -scale, 0);
	ret->MarkUpdate();
	return ret;
}

inline VertexBuffer *CreateCubeBuffer(VertexBuffer * ret)
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

