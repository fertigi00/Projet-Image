#include "RendererGDIPlus.h"

using namespace Gdiplus;

void RenderImageGDIPlus(HDC hdc, const RECT& rc, const LoadedImage& img)
{
    if (img.width <= 0 || img.height <= 0 || img.pixels.empty())
        return;

    const int w = img.width;
    const int h = img.height;

    Bitmap bitmap(w, h, w * 4, PixelFormat32bppARGB, (BYTE*)img.pixels.data());

    Graphics gfx(hdc);
    gfx.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gfx.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    gfx.SetSmoothingMode(SmoothingModeHighQuality);

    int destW = rc.right - rc.left;
    int destH = rc.bottom - rc.top;

    gfx.DrawImage(&bitmap,
        rc.left, rc.top,
        destW, destH);
}
