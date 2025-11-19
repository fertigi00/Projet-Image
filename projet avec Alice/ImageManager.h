#pragma once

#include <vector>
#include <cstdint>
#include <windows.h>

// Représente une image chargée en mémoire (format BGRA 32 bits)
struct LoadedImage
{
    int width;
    int height;
    std::vector<uint8_t> pixels; // B, G, R, A

    LoadedImage() : width(0), height(0) {}
};

// Initialisation / libération de GDI+
bool InitGDIPlus();
void ShutdownGDIPlus();

// Chargement / sauvegarde d'une image BMP 32 bits
bool LoadBMP(const wchar_t* path, LoadedImage& outImg);
bool SaveBMP(const wchar_t* path, const LoadedImage& img);

bool LoadImageAny(const wchar_t* path, LoadedImage& outImg);
bool SaveImageAny(const wchar_t* path, const LoadedImage& img);
