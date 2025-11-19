#include "StegEngine.h"
#include <vector>
#include <cstdint>

// Helpers LSB
static inline void WriteBitLSB(uint8_t& byte, uint8_t bit)
{
    byte = (byte & 0xFEu) | (bit & 1u);
}

static inline uint8_t ReadBitLSB(uint8_t byte)
{
    return byte & 1u;
}

static void BytesToBits(const std::vector<uint8_t>& inBytes, std::vector<uint8_t>& outBits)
{
    outBits.clear();
    outBits.reserve(inBytes.size() * 8);

    for (uint8_t b : inBytes)
    {
        for (int i = 7; i >= 0; --i)
        {
            uint8_t bit = (b >> i) & 1u;
            outBits.push_back(bit);
        }
    }
}

static void BitsToBytes(const std::vector<uint8_t>& bits, std::vector<uint8_t>& outBytes)
{
    outBytes.clear();
    size_t total = bits.size();

    for (size_t i = 0; i + 7 < total; i += 8)
    {
        uint8_t b = 0;
        for (int bit = 0; bit < 8; ++bit)
        {
            b = static_cast<uint8_t>((b << 1) | (bits[i + bit] & 1u));
        }
        outBytes.push_back(b);
    }
}

bool Steg::EmbedLSB(LoadedImage& img, const std::string& message)
{
    if (img.width <= 0 || img.height <= 0 || img.pixels.empty())
        return false;

    uint32_t length = static_cast<uint32_t>(message.size());

    std::vector<uint8_t> payload;

    // MAGIC
    payload.push_back((MAGIC >> 24) & 0xFFu);
    payload.push_back((MAGIC >> 16) & 0xFFu);
    payload.push_back((MAGIC >> 8) & 0xFFu);
    payload.push_back(MAGIC & 0xFFu);

    // LENGTH
    payload.push_back((length >> 24) & 0xFFu);
    payload.push_back((length >> 16) & 0xFFu);
    payload.push_back((length >> 8) & 0xFFu);
    payload.push_back(length & 0xFFu);

    // MESSAGE
    for (unsigned char c : message)
        payload.push_back(c);

    std::vector<uint8_t> bits;
    BytesToBits(payload, bits);

    size_t totalBits = bits.size();
    size_t totalPixels = static_cast<size_t>(img.width) * img.height;
    size_t capacity = totalPixels * 3u; // B, G, R

    if (totalBits > capacity)
        return false;

    size_t bitIndex = 0;

    for (size_t i = 0; i < totalPixels && bitIndex < totalBits; ++i)
    {
        uint8_t* px = &img.pixels[i * 4];

        WriteBitLSB(px[0], bits[bitIndex++]); // B
        if (bitIndex >= totalBits) break;

        WriteBitLSB(px[1], bits[bitIndex++]); // G
        if (bitIndex >= totalBits) break;

        WriteBitLSB(px[2], bits[bitIndex++]); // R
    }

    return true;
}

bool Steg::ExtractLSB(const LoadedImage& img, std::string& outMessage)
{
    outMessage.clear();

    if (img.width <= 0 || img.height <= 0 || img.pixels.empty())
        return false;

    size_t totalPixels = static_cast<size_t>(img.width) * img.height;

    std::vector<uint8_t> bits;
    bits.reserve(totalPixels * 3u);

    for (size_t i = 0; i < totalPixels; ++i)
    {
        const uint8_t* px = &img.pixels[i * 4];

        bits.push_back(ReadBitLSB(px[0]));
        bits.push_back(ReadBitLSB(px[1]));
        bits.push_back(ReadBitLSB(px[2]));
    }

    if (bits.size() < 64)
        return false;

    std::vector<uint8_t> headerBytes;
    BitsToBytes(std::vector<uint8_t>(bits.begin(), bits.begin() + 64), headerBytes);

    if (headerBytes.size() < 8)
        return false;

    uint32_t magic = 0;
    magic |= static_cast<uint32_t>(headerBytes[0]) << 24;
    magic |= static_cast<uint32_t>(headerBytes[1]) << 16;
    magic |= static_cast<uint32_t>(headerBytes[2]) << 8;
    magic |= static_cast<uint32_t>(headerBytes[3]);

    if (magic != MAGIC)
        return false;

    uint32_t length = 0;
    length |= static_cast<uint32_t>(headerBytes[4]) << 24;
    length |= static_cast<uint32_t>(headerBytes[5]) << 16;
    length |= static_cast<uint32_t>(headerBytes[6]) << 8;
    length |= static_cast<uint32_t>(headerBytes[7]);

    if (length == 0)
        return false;

    size_t headerBits = 64;
    size_t neededBits = headerBits + static_cast<size_t>(length) * 8u;

    if (neededBits > bits.size())
        return false;

    std::vector<uint8_t> msgBits(bits.begin() + headerBits, bits.begin() + neededBits);
    std::vector<uint8_t> msgBytes;
    BitsToBytes(msgBits, msgBytes);

    outMessage.assign(msgBytes.begin(), msgBytes.end());
    return true;
}
