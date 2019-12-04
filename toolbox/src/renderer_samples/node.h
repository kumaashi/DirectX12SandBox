#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <map>

struct node {
	std::string name = "";
	int update = 1;
	int type = 0;
	enum {
		T_NONE = 0,
		T_RENDERTARGET,
		T_TEXTURE,
		T_VERTEX,
		T_UNIT,
		T_VIEW,
		T_MAX,
	};
	node() {
	}
	node(std::string n) {
		name = n;
	}
	void set_type(int a) {
		type = a;
	}
	int get_type() {
		return type;
	}
	void mark_update(int a) {
		update = a;
	}
	int get_update() {
		return update;
	}
	
	void set_name(std::string n) {
		name = n;
	}
	std::string get_name() {
		return name;
	}
};

struct rendertarget : public node {
	int width, height;
	int mrt = 1;
	bool is_genmipmap = false;

	rendertarget(std::string name, int w, int h) :
		node(name), width(w), height(h)
	{
		set_type(T_RENDERTARGET);
	}

	bool get_genmipmap() {
		return is_genmipmap;
	}

	void set_genmipmap(bool v) {
		is_genmipmap = v;
	}

	int get_width() {
		return width;
	}
	int get_height() {
		return height;
	}
	int get_mrt() {
		return mrt;
	}
};

struct texture : public node {
	int width, height;
	std::vector<uint8_t> vdata;
	bool is_genmipmap = false;

	texture(std::string name, int w, int h, void *data, size_t size) :
		node(name), width(w), height(h)
	{
		set_type(T_TEXTURE);
		vdata.resize(size);
		memcpy(vdata.data(), data, size);
	}

	void *get_data() {
		return vdata.data();
	}
	size_t get_size() {
		return width * height * 4;
	}
	int get_width() {
		return width;
	}
	int get_height() {
		return height;
	}
	
	bool get_genmipmap() {
		return is_genmipmap;
	}

	void set_genmipmap(bool v) {
		is_genmipmap = v;
	}

};

struct vertex : public node {
	std::vector<uint8_t> vdata;
	size_t stride_size = 0;
	vertex(std::string name, void *data, size_t size, size_t ssize) :
		node(name)
	{
		set_type(T_VERTEX);
		vdata.resize(size);
		memcpy(vdata.data(), data, size);
		stride_size = ssize;
	}
	size_t get_stride_size() {
		return stride_size;
	}
	size_t get_size() {
		return vdata.size();
	}
	void *get_data() {
		return vdata.data();
	}
};

struct unit : public node {
	float m[16];
	std::string vertex_name;
	std::string texture_name;
	std::string shader_name;
	int vertex_num = 0;
	unit() {
	}
	unit(std::string name) : node(name) {
		set_type(T_UNIT);
		memset(m, 0, sizeof(m));
		m[0] = 1;
		m[5] = 1;
		m[10] = 1;
		m[15] = 1;
	}
	void set_pos(float x, float y, float z) {
		m[12] = x;
		m[13] = y;
		m[14] = z;
	}
	void set_scale(float x, float y, float z) {
		m[0] = x;
		m[5] = y;
		m[10] = z;
	}
	void set_vertex_name(std::string name) {
		vertex_name = name;
	}
	void set_texture_name(std::string name) {
		texture_name = name;
	}
	void set_shader_name(std::string name) {
		shader_name = name;
	}
	std::string get_vertex_name() {
		return vertex_name;
	}
	std::string get_texture_name() {
		return texture_name;
	}
	std::string get_shader_name() {
		return shader_name;
	}
	int get_vertex_num() {
		return vertex_num;
	}
	void set_vertex_num(int vnum) {
		vertex_num = vnum;
	}
	
};

struct view : public node {
	float ccol[4];
	int width, height;
	int order = 0;
	rendertarget *rt = nullptr;
	std::map<std::string, unit *> vunits;
	view(std::string name, int w, int h) :
		node(name), width(w), height(h)
	{
		set_type(T_VIEW);
		ccol[0] = 1.0f;
		ccol[1] = 0.0f;
		ccol[2] = 0.0f;
		ccol[3] = 1.0f;
	}

	bool operator < (const view & a) const
	{
		printf("order=%d, a.order=%d\n", order, a.order);
		exit(0);
		return order > a.order;
	};
	
	void set_unit(std::string & name, unit * u) {
		vunits[name] = u;
	}
	auto get_unit(std::string & name) {
		return vunits[name];
	}
	auto & get_units() {
		return vunits;
	}
	void set_order(int o) {
		order = o;
	}
	int get_order() {
		return order;
	}
	int get_width() {
		return width;
	}
	int get_height() {
		return height;
	}
	void set_clearcolor(float r, float g, float b, float a) {
		ccol[0] = r;
		ccol[1] = g;
		ccol[2] = b;
		ccol[3] = a;
	}
	void get_clearcolor(float *a) {
		a[0] = ccol[0];
		a[1] = ccol[1];
		a[2] = ccol[2];
		a[3] = ccol[3];
	}

	void set_rendertarget(rendertarget *r) {
		rt = r;
	}

	rendertarget *get_rendertarget() {
		return rt;
	}
};


