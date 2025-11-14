#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>

struct LoadedImage
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels; // BGRA 32 bits
};

// Initialisation et libération GDI+
bool InitGDIPlus();
void ShutdownGDIPlus();

// Chargement / Sauvegarde BMP
bool LoadBMPGDIPlus(const wchar_t* path, LoadedImage& outImg);
bool SaveBMPGDIPlus(const wchar_t* path, const LoadedImage& img);
