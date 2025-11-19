#pragma once
#include <windows.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "ImageManager.h"

namespace Steg
{
    // Alias LSB pour compatibilité
    bool EmbedMaster(LoadedImage& img, const std::string& msg);
    bool ExtractMaster(const LoadedImage& img, std::string& outMsg);

    inline bool EmbedLSB(LoadedImage& img, const std::string& msg) {
        return EmbedMaster(img, msg);
    }

    inline bool ExtractLSB(const LoadedImage& img, std::string& outMsg) {
        return ExtractMaster(img, outMsg);
    }

    // CRC32
    uint32_t ComputeCRC32(const uint8_t* data, size_t len);

    // MAGIC header
    static const uint32_t MAGIC = 0x4D534747; // "MSGG"
}
