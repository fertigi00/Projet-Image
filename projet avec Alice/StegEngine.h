#pragma once

#include <string>
#include "ImageManager.h"

namespace Steg
{
    constexpr unsigned int MAGIC = 0x53544547u;

    bool EmbedLSB(LoadedImage& img, const std::string& message);
    bool ExtractLSB(const LoadedImage& img, std::string& outMessage);
}
