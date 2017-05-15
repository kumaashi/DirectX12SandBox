#include "win.h"

static LRESULT WINAPI WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int temp = wParam & 0xFFF0;
	switch (msg) {
	case WM_SYSCOMMAND:
		if (temp == SC_MONITORPOWER || temp == SC_SCREENSAVE) {
			return 0;
		}
		break;
	case WM_IME_SETCONTEXT:
		lParam &= ~ISC_SHOWUIALL;
		return 0;
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostQuitMessage(0);
		break;
	case WM_SIZE:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND win_init(const char *appname, int Width, int Height) {
	static WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX), CS_CLASSDC | CS_VREDRAW | CS_HREDRAW,
		WindowProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(NULL, IDI_APPLICATION),
		LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		NULL, appname, NULL
	};
	wcex.hInstance = GetModuleHandle(NULL);
	RegisterClassEx(&wcex);

	DWORD styleex = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style = WS_OVERLAPPEDWINDOW;
	style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

	RECT rc;
	SetRect( &rc, 0, 0, Width, Height );
	AdjustWindowRectEx( &rc, style, FALSE, styleex);
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	HWND  hWnd = CreateWindowEx(styleex, appname, appname, style, 0, 0, rc.right, rc.bottom, NULL, NULL, wcex.hInstance, NULL);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	ShowCursor(TRUE);
	return hWnd;
}

BOOL win_proc_msg() {
	MSG msg;
	BOOL IsActive = TRUE;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) IsActive = FALSE;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return IsActive;
}
