#include "ImageManager.h"
#include <gdiplus.h>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static ULONG_PTR g_gdiToken = 0;

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
        if (!src || !dst)
            continue;
        memcpy(dst, src, w * 4);
    }

    bmp.UnlockBits(&data);
    return true;
}

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
        if (!src || !dst)
            continue;
        memcpy(dst, src, img.width * 4);
    }

    bmp.UnlockBits(&data);

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
        FindCodec(L"image/bmp", clsid);

    return bmp.Save(path, &clsid, nullptr) == Gdiplus::Ok;
}
