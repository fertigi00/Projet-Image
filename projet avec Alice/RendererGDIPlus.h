#pragma once

#include <windows.h>
#include <gdiplus.h>
#include "ImageManager.h"

void RenderImageGDIPlus(HDC hdc, const RECT& rc, const LoadedImage& img);
