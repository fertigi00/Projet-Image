#pragma once

#include <vector>
#include <cstdint>
#include <windows.h>

struct LoadedImage
{
    int width;
    int height;
    std::vector<uint8_t> pixels; // B, G, R, A

    LoadedImage() : width(0), height(0) {}
};

bool InitGDIPlus();
void ShutdownGDIPlus();

bool LoadImageAny(const wchar_t* path, LoadedImage& outImg);
bool SaveImageAny(const wchar_t* path, const LoadedImage& img);
