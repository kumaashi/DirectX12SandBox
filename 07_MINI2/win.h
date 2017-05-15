#pragma once

#include <windows.h>

#pragma  comment(lib, "gdi32.lib")
#pragma  comment(lib, "winmm.lib")
#pragma  comment(lib, "user32.lib")
#pragma  comment(lib, "glu32.lib")

HWND win_init(const char *appname, int Width, int Height);
BOOL win_proc_msg();

