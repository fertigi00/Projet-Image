#include "RendererGDI.h"

void RenderImageGDI(HDC hdc, const RECT& rc, const LoadedImage* img)
{
    if (!img || img->pixels.empty()) // <--- correction ici
        return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = img->width;
    bmi.bmiHeader.biHeight = -img->height;   // negative = top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;           // BGRA
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(
        hdc,
        rc.left,
        rc.top,
        rc.right - rc.left,
        rc.bottom - rc.top,
        0,
        0,
        img->width,
        img->height,
        img->pixels.data(),  // <--- correction ici
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
