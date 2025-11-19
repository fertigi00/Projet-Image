#pragma once

#include <string>
#include "ImageManager.h"

// Format : [4 octets MAGIC][4 octets LENGTH][LENGTH octets message]
// MAGIC = 0x53544547 ("STEG")

namespace Steg
{
    constexpr unsigned int MAGIC = 0x53544547u;

    bool EmbedLSB(LoadedImage& img, const std::string& message);
    bool ExtractLSB(const LoadedImage& img, std::string& outMessage);
}
