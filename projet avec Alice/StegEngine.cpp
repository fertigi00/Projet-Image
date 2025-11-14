#include "StegEngine.h"
#include <vector>

using namespace Steg;

// ---- Helpers ----

// Write 1 bit in LSB of a byte
static inline void WriteBitLSB(uint8_t& byte, uint8_t bit) {
    byte = (byte & 0xFE) | (bit & 1);
}

// Read 1 bit from LSB
static inline uint8_t ReadBitLSB(uint8_t byte) {
    return byte & 1;
}

// Convert bytes -> bits
static void BytesToBits(const std::vector<uint8_t>& in, std::vector<uint8_t>& outBits) {
    for (uint8_t b : in) {
        for (int i = 7; i >= 0; i--)
            outBits.push_back((b >> i) & 1);
    }
}

// Convert bits -> bytes
static void BitsToBytes(const std::vector<uint8_t>& bits, std::vector<uint8_t>& outBytes) {
    size_t total = bits.size();
    for (size_t i = 0; i + 7 < total; i += 8) {
        uint8_t b = 0;
        for (int bit = 0; bit < 8; bit++)
            b = (b << 1) | (bits[i + bit] & 1);
        outBytes.push_back(b);
    }
}

// ---- Embed message ----
bool Steg::EmbedLSB(LoadedImage& img, const std::string& msg) {
    if (img.pixels.empty()) return false;

    uint32_t length = static_cast<uint32_t>(msg.size());
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
    for (char c : msg) payload.push_back(static_cast<uint8_t>(c));

    // Convert to bits
    std::vector<uint8_t> bits;
    BytesToBits(payload, bits);

    size_t totalBits = bits.size();
    size_t pixels = img.width * img.height;
    if (totalBits > pixels * 3) return false; // capacity check

    // Embed bits in BGR channels
    size_t bitIndex = 0;
    for (size_t i = 0; i < pixels && bitIndex < totalBits; i++) {
        uint8_t* px = &img.pixels[i * 4]; // BGRA
        WriteBitLSB(px[0], bits[bitIndex++]); // B
        if (bitIndex >= totalBits) break;
        WriteBitLSB(px[1], bits[bitIndex++]); // G
        if (bitIndex >= totalBits) break;
        WriteBitLSB(px[2], bits[bitIndex++]); // R
    }
    return true;
}

// ---- Extract message ----
bool Steg::ExtractLSB(const LoadedImage& img, std::string& outMsg) {
    if (img.pixels.empty()) return false;

    size_t pixels = img.width * img.height;
    std::vector<uint8_t> bits;
    bits.reserve(pixels * 3);

    // Read bits
    for (size_t i = 0; i < pixels; i++) {
        const uint8_t* px = &img.pixels[i * 4];
        bits.push_back(ReadBitLSB(px[0]));
        bits.push_back(ReadBitLSB(px[1]));
        bits.push_back(ReadBitLSB(px[2]));
    }

    // Convert only first 64 bits for header
    std::vector<uint8_t> headerBytes;
    BitsToBytes(std::vector<uint8_t>(bits.begin(), bits.begin() + 64), headerBytes);

    if (headerBytes.size() < 8) return false;

    uint32_t magic = (headerBytes[0] << 24) | (headerBytes[1] << 16) | (headerBytes[2] << 8) | headerBytes[3];
    if (magic != MAGIC) return false;

    uint32_t length = (headerBytes[4] << 24) | (headerBytes[5] << 16) | (headerBytes[6] << 8) | headerBytes[7];
    if (length == 0 || 8 + length > (bits.size() / 8)) return false;

    // Extract message bits
    std::vector<uint8_t> msgBits(bits.begin() + 64, bits.begin() + 64 + length * 8);
    std::vector<uint8_t> msgBytes;
    BitsToBytes(msgBits, msgBytes);

    outMsg.assign(msgBytes.begin(), msgBytes.end());
    return true;
}
