#include "StegEngine.h"
#include <vector>

using namespace Steg;

//
// Helper: write 1 bit into 1 byte LSB
//
static inline void WriteBitLSB(uint8_t& byte, uint8_t bit)
{
    byte = (byte & 0xFE) | (bit & 1);
}

//
// Helper: read 1 bit from 1 byte LSB
//
static inline uint8_t ReadBitLSB(uint8_t byte)
{
    return byte & 1;
}

//
// Convert bytes to a vector of bits
//
static void BytesToBits(const std::vector<uint8_t>& in, std::vector<uint8_t>& outBits)
{
    for (uint8_t b : in)
    {
        for (int i = 7; i >= 0; i--)
            outBits.push_back((b >> i) & 1);
    }
}

//
// Convert bits to bytes
//
static void BitsToBytes(const std::vector<uint8_t>& bits, std::vector<uint8_t>& outBytes)
{
    size_t total = bits.size();
    for (size_t i = 0; i + 7 < total; i += 8)
    {
        uint8_t b = 0;
        for (int bit = 0; bit < 8; bit++)
            b = (b << 1) | (bits[i + bit] & 1);

        outBytes.push_back(b);
    }
}

//
// Embedding: MAGIC + LENGTH + TEXT
//
bool Steg::EmbedLSB(LoadedImage& img, const std::string& msg)
{
    if (img.pixels.empty())
        return false;

    uint32_t length = static_cast<uint32_t>(msg.size());

    // Build header + data buffer
    std::vector<uint8_t> payload;

    // MAGIC
    uint32_t magic = MAGIC;
    payload.push_back((magic >> 24) & 0xFF);
    payload.push_back((magic >> 16) & 0xFF);
    payload.push_back((magic >> 8) & 0xFF);
    payload.push_back((magic >> 0) & 0xFF);

    // LENGTH
    payload.push_back((length >> 24) & 0xFF);
    payload.push_back((length >> 16) & 0xFF);
    payload.push_back((length >> 8) & 0xFF);
    payload.push_back((length >> 0) & 0xFF);

    // MESSAGE
    for (char c : msg)
        payload.push_back(static_cast<uint8_t>(c));

    // Convert payload to bit stream
    std::vector<uint8_t> bits;
    BytesToBits(payload, bits);
    size_t totalBits = bits.size();

    // Check capacity
    size_t pixels = img.width * img.height;
    if (totalBits > pixels * 3) // 3 LSB per pixel (B, G, R)
        return false;

    // Embed bits into pixels
    size_t bitIndex = 0;
    for (size_t i = 0; i < pixels && bitIndex < totalBits; i++)
    {
        uint8_t* px = &img.pixels[i * 4]; // pointer sur le pixel (B,G,R,A)

        WriteBitLSB(px[0], bits[bitIndex++]); // B
        if (bitIndex >= totalBits) break;

        WriteBitLSB(px[1], bits[bitIndex++]); // G
        if (bitIndex >= totalBits) break;

        WriteBitLSB(px[2], bits[bitIndex++]); // R
    }

    return true;
}

//
// Extract message
//
bool Steg::ExtractLSB(const LoadedImage& img, std::string& outMsg)
{
    if (img.pixels.empty())
        return false;

    size_t pixels = img.width * img.height;
    std::vector<uint8_t> bits;
    bits.reserve(pixels * 3);

    // Read all LSBs
    for (size_t i = 0; i < pixels; i++)
    {
        const uint8_t* px = &img.pixels[i * 4];
        bits.push_back(ReadBitLSB(px[0])); // B
        bits.push_back(ReadBitLSB(px[1])); // G
        bits.push_back(ReadBitLSB(px[2])); // R
    }

    // Convert bits to bytes
    std::vector<uint8_t> bytes;
    BitsToBytes(bits, bytes);

    if (bytes.size() < 8)
        return false;

    // Check MAGIC
    uint32_t magic =
        (bytes[0] << 24) |
        (bytes[1] << 16) |
        (bytes[2] << 8) |
        (bytes[3]);

    if (magic != MAGIC)
        return false;

    // Read LENGTH
    uint32_t length =
        (bytes[4] << 24) |
        (bytes[5] << 16) |
        (bytes[6] << 8) |
        (bytes[7]);

    if (bytes.size() < 8 + length)
        return false;

    // Extract MESSAGE
    outMsg.clear();
    for (uint32_t i = 0; i < length; i++)
        outMsg.push_back(static_cast<char>(bytes[8 + i]));

    return true;
}
