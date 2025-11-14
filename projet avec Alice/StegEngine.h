#pragma once
#include <windows.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "ImageManager.h"

namespace Steg
{
    bool EmbedLSB(LoadedImage& img, const std::string& msg);
    bool ExtractLSB(const LoadedImage& img, std::string& outMsg);

    static const uint32_t MAGIC = 0x4D534747; // "MSGG"
}
