#pragma once
#include <windows.h>
#include "ImageManager.h" // pour LoadedImage

// WNDPROC
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Image globale (LoadedImage)
extern LoadedImage gImage;
