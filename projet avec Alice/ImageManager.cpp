#include "ImageManager.h"
#include <gdiplus.h>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static ULONG_PTR g_gdiToken = 0;

// ---------------------------------------------------------
// GDI+ : initialisation / libération
// ---------------------------------------------------------
bool InitGDIPlus()
{
    if (g_gdiToken != 0)
        return true;

    GdiplusStartupInput input{};
    return GdiplusStartup(&g_gdiToken, &input, nullptr) == Ok;
}

void ShutdownGDIPlus()
{
    if (g_gdiToken != 0)
    {
        GdiplusShutdown(g_gdiToken);
        g_gdiToken = 0;
    }
}

// ---------------------------------------------------------
// Chargement BMP
// ---------------------------------------------------------
bool LoadBMP(const wchar_t* path, LoadedImage& outImg)
{
    outImg = LoadedImage();

    Bitmap bmp(path);
    if (bmp.GetLastStatus() != Ok)
        return false;

    int w = bmp.GetWidth();
    int h = bmp.GetHeight();
    if (w <= 0 || h <= 0)
        return false;

    BitmapData data{};
    Rect rect(0, 0, w, h);

    if (bmp.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &data) != Ok)
        return false;

    outImg.width = w;
    outImg.height = h;
    outImg.pixels.resize(w * h * 4);

    for (int y = 0; y < h; ++y)
    {
        uint8_t* src = reinterpret_cast<uint8_t*>(data.Scan0) + y * data.Stride;
        uint8_t* dst = outImg.pixels.data() + y * w * 4;
        memcpy(dst, src, w * 4);
    }

    bmp.UnlockBits(&data);
    return true;
}

// ---------------------------------------------------------
// Sauvegarde BMP
// ---------------------------------------------------------
bool SaveBMP(const wchar_t* path, const LoadedImage& img)
{
    if (img.width <= 0 || img.height <= 0 || img.pixels.empty())
        return false;

    Bitmap bmp(img.width, img.height, PixelFormat32bppARGB);

    BitmapData data{};
    Rect rect(0, 0, img.width, img.height);

    if (bmp.LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &data) != Ok)
        return false;

    for (int y = 0; y < img.height; ++y)
    {
        uint8_t* dst = reinterpret_cast<uint8_t*>(data.Scan0) + y * data.Stride;
        const uint8_t* src = img.pixels.data() + y * img.width * 4;
        memcpy(dst, src, img.width * 4);
    }

    bmp.UnlockBits(&data);

    // Trouver le codec BMP
    UINT numEncoders = 0, size = 0;
    GetImageEncodersSize(&numEncoders, &size);
    if (size == 0)
        return false;

    std::vector<BYTE> buffer(size);
    ImageCodecInfo* encoders = reinterpret_cast<ImageCodecInfo*>(buffer.data());
    GetImageEncoders(numEncoders, size, encoders);

    CLSID clsid{};
    bool found = false;
    for (UINT i = 0; i < numEncoders; ++i)
    {
        if (wcscmp(encoders[i].MimeType, L"image/bmp") == 0)
        {
            clsid = encoders[i].Clsid;
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    return bmp.Save(path, &clsid, nullptr) == Ok;
}

// ---------------------------------------------------------
// Charge n'importe quel format connu par GDI+
// BMP / PNG / JPG / GIF / TIFF
// ---------------------------------------------------------
bool LoadImageAny(const wchar_t* path, LoadedImage& outImg)
{
    outImg = LoadedImage();

    Gdiplus::Bitmap bmp(path);
    if (bmp.GetLastStatus() != Gdiplus::Ok)
        return false;

    int w = bmp.GetWidth();
    int h = bmp.GetHeight();
    if (w <= 0 || h <= 0)
        return false;

    Gdiplus::Rect rect(0, 0, w, h);
    Gdiplus::BitmapData data{};

    if (bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
        return false;

    outImg.width = w;
    outImg.height = h;
    outImg.pixels.resize(w * h * 4);

    for (int y = 0; y < h; y++)
    {
        uint8_t* dst = outImg.pixels.data() + y * w * 4;
        uint8_t* src = (uint8_t*)data.Scan0 + y * data.Stride;
        memcpy(dst, src, w * 4);
    }

    bmp.UnlockBits(&data);
    return true;
}

// ---------------------------------------------------------
// Sauvegarde dans le format correspondant à l'extension :
//   .bmp → codec BMP
//   .png → codec PNG
//   .jpg .jpeg → codec JPEG
// ---------------------------------------------------------
bool SaveImageAny(const wchar_t* path, const LoadedImage& img)
{
    if (img.width <= 0 || img.height <= 0 || img.pixels.empty())
        return false;

    Gdiplus::Bitmap bmp(img.width, img.height, PixelFormat32bppARGB);

    Gdiplus::Rect rect(0, 0, img.width, img.height);
    Gdiplus::BitmapData data{};

    if (bmp.LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
        return false;

    for (int y = 0; y < img.height; y++)
    {
        uint8_t* dst = (uint8_t*)data.Scan0 + y * data.Stride;
        const uint8_t* src = img.pixels.data() + y * img.width * 4;
        memcpy(dst, src, img.width * 4);
    }

    bmp.UnlockBits(&data);

    // Choisir le codec selon l’extension
    const wchar_t* ext = wcsrchr(path, L'.');
    CLSID clsid{};

    auto FindCodec = [&](const wchar_t* mime, CLSID& out)
        {
            UINT num = 0, size = 0;
            GetImageEncodersSize(&num, &size);
            if (!size) return false;

            std::vector<BYTE> buffer(size);
            auto* pInfo = reinterpret_cast<ImageCodecInfo*>(buffer.data());
            GetImageEncoders(num, size, pInfo);

            for (UINT i = 0; i < num; i++)
            {
                if (wcscmp(pInfo[i].MimeType, mime) == 0)
                {
                    out = pInfo[i].Clsid;
                    return true;
                }
            }
            return false;
        };

    if (!ext)
        return false;

    if (_wcsicmp(ext, L".png") == 0)
        FindCodec(L"image/png", clsid);
    else if (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0)
        FindCodec(L"image/jpeg", clsid);
    else
        FindCodec(L"image/bmp", clsid);  // Par défaut BMP

    return bmp.Save(path, &clsid, nullptr) == Gdiplus::Ok;
}
