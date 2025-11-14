#pragma once
#include <windows.h>
#include "ImageManager.h"

void RenderImageGDI(HDC hdc, const RECT& rc, const LoadedImage* img);
