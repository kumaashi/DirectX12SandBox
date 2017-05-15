#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
#include <DirectXMath.h>

enum {
	BUFFER_TYPE_NONE = 0,
	BUFFER_TYPE_SHADER,
	BUFFER_TYPE_VERTEX,
	BUFFER_TYPE_TEXTURE,
	BUFFER_TYPE_RENDERTARGET,
	BUFFER_TYPE_INSTANCING,
	BUFFER_TYPE_VIEW,
};

struct VectorData {
	float data[4];
};

struct MatrixData {
	float data[16];
	void Print() {
		for (int i = 0; i < 16; i++) {
			if ( (i % 4) == 0) printf("\n");
			printf("%f, ", data[i]);
		}
	}
};


class BaseBuffer {
	std::string name;
	int type;
	bool is_update;
public:
	BaseBuffer() {
		name = "";
		is_update = false;
	}

	virtual ~BaseBuffer() {
	}

	void SetType(int t) {
		type = t;
	}

	int GetType() {
		return type;
	}

	std::string GetName() {
		return name;
	}

	void SetName(const std::string &s) {
		name = s;
	}

	void MarkUpdate() {
		is_update = true;
	}

	void UnmarkUpdate() {
		is_update = false;
	}

	bool IsUpdate() {
		return is_update;
	}
};


class VertexBuffer;
struct InstancingBuffer : public BaseBuffer {
public:
	struct Data {
		MatrixData world;
		VectorData color;
	};
	InstancingBuffer() {
		SetType(BUFFER_TYPE_INSTANCING);
		vdata.clear();
	}
	virtual ~InstancingBuffer() {
	}
	void ClearData() {
		vdata.clear();
	}
	void AddData(const Data &data) {
		vdata.push_back(data);
	}
	void GetData(std::vector<Data> &dest) {
		dest = vdata;
	}
	void SetVertexBuffer(std::string &name, VertexBuffer *a) {
		vmap[name] = a;
	}

	void DelVertexBuffer(std::string &a) {
		vmap.erase(a);
	}

	void GetVertexBuffer(std::vector<VertexBuffer *> &dest) {
		auto it = vmap.begin();
		auto ite = vmap.end();
		while (it != ite) {
			dest.push_back(it->second);
			it++;
		}
	}

private:
	//position and color list
	std::vector<Data> vdata;

	//draw geometry lists
	std::map<std::string, VertexBuffer *> vmap;
};

class TextureBuffer : public BaseBuffer {
	int width;
	int height;
	int format;
	std::vector<char> buffer;
public:
	TextureBuffer() {
		SetType(BUFFER_TYPE_TEXTURE);
		width = 0;
		height = 0;
		format = 0;
	}

	virtual ~TextureBuffer() {
	}

	void Create(int w, int h, int fmt) {
		width = w;
		height = h;
		format = fmt;
	}

	void SetData(void *data, size_t size) {
		buffer.resize(size);
		memcpy(&buffer[0], data, size);
		MarkUpdate();
	}

	bool GetData(std::vector<char> &dest) {
		if (!buffer.empty()) {
			std::copy(buffer.begin(), buffer.end(), std::back_inserter(dest) );
			return true;
		}
		return false;
	}

	void *GetBuffer() {
		if (!buffer.empty()) {
			return &buffer[0];
		}
		return 0;
	}

	int GetWidth() {
		return width;
	}
	int GetHeight() {
		return height;
	}
	int GetFormat() {
		return format;
	}
};

class RenderTargetBuffer : public BaseBuffer {
	enum {
		MAX = 4,
	};
	TextureBuffer color_texture[MAX];
	TextureBuffer depth_texture;
	int width;
	int height;
	int format;
	int type;
	int count;
	float clear_color[4];
public:
	enum {
		TypeBackBuffer = 0,
		TypeTargetBuffer,
		TypeMax,
	};
	RenderTargetBuffer() {
		SetType(BUFFER_TYPE_RENDERTARGET);
		width = 0;
		height = 0;
		format = 0;
		type = 0;
		clear_color[0] = 1.0f;
		clear_color[1] = 0.0f;
		clear_color[2] = 1.0f;
		clear_color[3] = 1.0f;
	}

	virtual ~RenderTargetBuffer() {
	}

	void Create(int w, int h, int fmt, int t, int cnt) {
		width = w;
		height = h;
		format = fmt;
		type = t;
		count = cnt;
		for (int i = 0 ; i < MAX; i++) {
			char name[0x120];
			sprintf(name, "_%d", i);
			color_texture[i].SetName(GetName() + std::string(name));
			color_texture[i].Create(w, h, fmt);
		}
		{
			depth_texture.SetName(GetName() + std::string("_depth"));
			depth_texture.Create(w, h, fmt);
		}
	}
	int GetWidth() {
		return width;
	}
	int GetHeight() {
		return height;
	}
	int GetFormat() {
		return format;
	}
	int GetType() {
		return type;
	}

	TextureBuffer *GetTexture(int i, bool is_depth = false) {
		if (is_depth) {
			return &depth_texture;
		}
		return &color_texture[i];
	}

	const int GetCount() {
		return count;
	}

	void SetClearColor(float r, float g, float b, float a) {
		clear_color[0] = r;
		clear_color[1] = g;
		clear_color[2] = b;
		clear_color[3] = a;
	}

	void GetClearColor(float *a) {
		for (int i = 0; i < 4; i++) {
			a[i] = clear_color[i];
		}
	}
};


class ViewBuffer : public BaseBuffer {
	MatrixData view;
	MatrixData proj;
	MatrixData prev_view;
	MatrixData prev_proj;

	RenderTargetBuffer *rendertarget;
	float vp_x;
	float vp_y;
	float vp_w;
	float vp_h;
	float clearcolor[4];
	float effectparam[4];
	int order;
	std::map<std::string, InstancingBuffer *> vimap;
public:
	ViewBuffer() {
		SetType(BUFFER_TYPE_VIEW);
		rendertarget = 0;
		vp_x = 0;
		vp_y = 0;
		vp_w = 0;
		vp_h = 0;
		order = 0;
	}

	virtual ~ViewBuffer() {
		rendertarget = 0;
	}

	void SetInstancingBuffer(std::string &name, InstancingBuffer *a) {
		vimap[name] = a;
	}

	void DelInstancingBuffer(std::string &a) {
		vimap.erase(a);
	}

	void GetInstancingBuffer(std::vector<InstancingBuffer *> &dest) {
		for (auto & it : vimap) {
			dest.push_back(it.second);
		}
	}

	void GetClearColor(float a[4]) {
		for (int i = 0 ; i < 4; i++) {
			a[i] = clearcolor[i];
		}
	}

	void SetClearColor(float r, float g, float b, float a) {
		clearcolor[0] = r;
		clearcolor[1] = g;
		clearcolor[2] = b;
		clearcolor[3] = a;
	}


	void GetEffectParam(float a[4]) {
		for (int i = 0 ; i < 4; i++) {
			a[i] = effectparam[i];
		}
	}
	void SetEffectParam(float x, float y, float z, float w) {
		effectparam[0] = x;
		effectparam[1] = y;
		effectparam[2] = z;
		effectparam[3] = w;
	}

	void SetOrder(int n) {
		order = n;
	}

	int GetOrder() {
		return order;
	}

	MatrixData GetViewMatrix() {
		return view;
	}

	MatrixData GetProjMatrix() {
		return proj;
	}

	MatrixData GetPrevViewMatrix() {
		return prev_view;
	}

	MatrixData GetPrevProjMatrix() {
		return prev_proj;
	}

	void SetViewMatrix(MatrixData &m) {
		prev_view = view;
		view = m;
	}

	void SetProjMatrix(MatrixData &m) {
		prev_proj = proj;
		proj = m;
	}

	float GetX() {
		return vp_x;
	}

	float GetY() {
		return vp_y;
	}

	float GetW() {
		return vp_w;
	}

	float GetH() {
		return vp_h;
	}

	void SetViewPort(float x, float y, float w, float h) {
		vp_x = x;
		vp_y = y;
		vp_w = w;
		vp_h = h;
	}

	void SetRenderTarget(RenderTargetBuffer *p) {
		rendertarget = p;
	}

	RenderTargetBuffer *GetRenderTarget() {
		return rendertarget;
	}
};

class VertexBuffer : public BaseBuffer {
public:
	typedef enum {
		TRIANGLES = 0,
		PATCHES,
	} Topologie;
	enum {
		Vertex = 0,
		Normal,
		Uv,
		Color,
		Max,
	};
	struct Format {
		float pos  [3] ;
		float nor  [3] ;
		float uv   [2] ;
		float color[4] ;
	};
	VertexBuffer() {
		SetType(BUFFER_TYPE_VERTEX);
		topologie = TRIANGLES;
	}

	virtual ~VertexBuffer() {
	}

	void SetMatrix(const std::string &name, const MatrixData &e) {
		uniform_mat4[name] = e;
	}

	void GetMatrix(std::map<std::string, MatrixData> &m) {
		auto it = uniform_mat4.begin();
		auto ite = uniform_mat4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

	void SetVector(const std::string &name, const VectorData &e) {
		uniform_vec4[name] = e;
	}

	void GetVector(std::map<std::string, VectorData> &m) {
		auto it = uniform_vec4.begin();
		auto ite = uniform_vec4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}
	void Create(int size) {
		buffer.resize(size);
		memset(&buffer[0], 0, sizeof(Format) * size);
	}

	void SetPos(int index, float x, float y, float z) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].pos[0] = x;
		buffer[index].pos[1] = y;
		buffer[index].pos[2] = z;
	}
	void SetNormal(int index, float x, float y, float z) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].nor[0] = x;
		buffer[index].nor[1] = y;
		buffer[index].nor[2] = z;
	}

	void SetUv(int index, float u, float v) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].uv[0] = u;
		buffer[index].uv[1] = v;
	}

	void SetColor(int index, float x, float y, float z, float w) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].color[0] = x;
		buffer[index].color[1] = y;
		buffer[index].color[2] = z;
		buffer[index].color[3] = w;
	}

	VertexBuffer::Format *GetBuffer() {
		return &buffer[0];
	}

	size_t GetBufferSize() {
		return buffer.size() * sizeof(VertexBuffer::Format);
	}

	size_t GetCount() {
		return buffer.size();
	}

	void SetShaderName(const std::string &name) {
		shader_name0 = name;
	}

	std::string GetShaderName() {
		return shader_name0;
	}

	Topologie GetTopologie() {
		return topologie;
	}

	void SetTopologie(Topologie t) {
		topologie = t;
	}

	void SetTextureName(const std::string &texname, TextureBuffer *tex) {
		if (tex) {
			uniform_texture_name[texname] = tex->GetName();
		}
	}

	void SetTextureIndex(int idx, TextureBuffer *tex) {
		if (tex) {
			uniform_texture_index[idx] = tex->GetName();
		}
	}

	void GetTextureName(std::map<std::string, std::string> &m) {
		auto it = uniform_texture_name.begin();
		auto ite = uniform_texture_name.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}
	void GetTextureIndex(std::map<int, std::string> &m) {
		auto it = uniform_texture_index.begin();
		auto ite = uniform_texture_index.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

private:
	Topologie topologie;
	std::vector<Format> buffer;
	std::map<std::string, MatrixData> uniform_mat4;
	std::map<std::string, VectorData> uniform_vec4;
	std::map<std::string, std::string> uniform_texture_name;
	std::map<int, std::string> uniform_texture_index;
	std::string shader_name0;
};

