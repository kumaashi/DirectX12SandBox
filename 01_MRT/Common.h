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

#pragma  warning(disable:4996)
#pragma  warning(disable:4313)
#pragma  warning(disable:4477)

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

