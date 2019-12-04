#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>
#include <dwmapi.h>
#include <windows.h>
#include <dxgi1_4.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxgi.lib")

struct winapp {
	HWND hwnd = nullptr;
	HINSTANCE instance = nullptr;

	static LRESULT WINAPI
	msg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_SYSCOMMAND:
			switch ((wParam & 0xFFF0)) {
			case SC_MONITORPOWER:
			case SC_SCREENSAVE:
				return 0;
			default:
				break;
			}
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_IME_SETCONTEXT:
			lParam &= ~ISC_SHOWUIALL;
			break;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
				PostQuitMessage(0);
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}


	int
	create(const char *name, int w, int h)
	{
		instance = GetModuleHandle(NULL);
		auto style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
		auto ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		RECT rc = {0, 0, w, h};
		WNDCLASSEX twc = {
			sizeof(WNDCLASSEX), CS_CLASSDC, msg_proc, 0L, 0L, instance,
			LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW),
			(HBRUSH)GetStockObject(BLACK_BRUSH), NULL, name, NULL
		};

		RegisterClassEx(&twc);
		AdjustWindowRectEx(&rc, style, FALSE, ex_style);
		rc.right -= rc.left;
		rc.bottom -= rc.top;
		hwnd = CreateWindowEx(
			ex_style, name, name, style,
			/*
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			*/
			0, 0,
			rc.right, rc.bottom, NULL, NULL, instance, NULL);
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
		return (0);
	};
	
	HWND
	get_handle() {
		return hwnd;
	}

	HINSTANCE
	get_instance() {
		return instance;
	}

	int
	update()
	{
		MSG msg;
		int is_active = 1;

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				is_active = 0;
				break;
			} else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return is_active;
	}

};
