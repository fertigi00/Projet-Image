#include "ImageManager.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static ULONG_PTR gdiToken = 0;

// ----------------------------
// Initialisation / libération GDI+
// ----------------------------
bool InitGDIPlus()
{
    if (gdiToken) return true;

    GdiplusStartupInput gdiplusStartupInput;
    return GdiplusStartup(&gdiToken, &gdiplusStartupInput, nullptr) == Ok;
}

void ShutdownGDIPlus()
{
    if (gdiToken) {
        GdiplusShutdown(gdiToken);
        gdiToken = 0;
    }
}

// ----------------------------
// Chargement BMP
// ----------------------------
bool LoadBMPGDIPlus(const wchar_t* path, LoadedImage& outImg)
{
    outImg = LoadedImage();

    Bitmap bitmap(path);
    if (bitmap.GetLastStatus() != Ok) return false;

    int width = bitmap.GetWidth();
    int height = bitmap.GetHeight();

    // Lock bits pour obtenir un pointeur vers les pixels
    BitmapData bmpData;
    Gdiplus::Rect rect(0, 0, width, height);
    if (bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != Ok)
        return false;

    outImg.width = width;
    outImg.height = height;
    outImg.pixels.resize(width * height * 4);

    // Copier ligne par ligne (GDI+ stocke en BGRA 32 bits)
    for (int y = 0; y < height; y++)
    {
        uint8_t* src = reinterpret_cast<uint8_t*>(bmpData.Scan0) + y * bmpData.Stride;
        uint8_t* dst = outImg.pixels.data() + y * width * 4;
        memcpy(dst, src, width * 4);
    }

    bitmap.UnlockBits(&bmpData);
    return true;
}

// ----------------------------
// Sauvegarde BMP (32 bits)
// ----------------------------
bool SaveBMPGDIPlus(const wchar_t* path, const LoadedImage& img)
{
    if (img.pixels.empty()) return false;

    Bitmap bitmap(img.width, img.height, PixelFormat32bppARGB);

    BitmapData bmpData;
    Gdiplus::Rect rect(0, 0, img.width, img.height);
    if (bitmap.LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bmpData) != Ok)
        return false;

    for (int y = 0; y < img.height; y++)
    {
        uint8_t* dst = reinterpret_cast<uint8_t*>(bmpData.Scan0) + y * bmpData.Stride;
        const uint8_t* src = img.pixels.data() + y * img.width * 4;
        memcpy(dst, src, img.width * 4);
    }

    bitmap.UnlockBits(&bmpData);

    CLSID clsid;
    // Trouver le codec BMP
    UINT numEncoders = 0, size = 0;
    GetImageEncodersSize(&numEncoders, &size);
    if (size == 0) return false;

    std::vector<BYTE> buffer(size);
    ImageCodecInfo* encoders = reinterpret_cast<ImageCodecInfo*>(buffer.data());
    GetImageEncoders(numEncoders, size, encoders);

    for (UINT i = 0; i < numEncoders; i++)
    {
        if (wcscmp(encoders[i].MimeType, L"image/bmp") == 0)
        {
            clsid = encoders[i].Clsid;
            break;
        }
    }

    return bitmap.Save(path, &clsid, nullptr) == Ok;
}