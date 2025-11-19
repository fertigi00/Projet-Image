#pragma once

#include <windows.h>
#include <gdiplus.h>
#include "ImageManager.h"

// Affichage image via GDI+
void RenderImageGDIPlus(HDC hdc, const RECT& rc, const LoadedImage& img);
