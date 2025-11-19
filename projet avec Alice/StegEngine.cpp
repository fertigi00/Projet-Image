#include "StegEngine.h"
#include <vector>

using namespace Steg;

// ---- Helpers ----
static inline void WriteBitLSB(uint8_t& byte, uint8_t bit) {
    byte = (byte & 0xFE) | (bit & 1);
}

static inline uint8_t ReadBitLSB(uint8_t byte) {
    return byte & 1;
}

static void BytesToBits(const std::vector<uint8_t>& in, std::vector<uint8_t>& outBits) {
    for (uint8_t b : in)
        for (int i = 7; i >= 0; i--)
            outBits.push_back((b >> i) & 1);
}

static void BitsToBytes(const std::vector<uint8_t>& bits, std::vector<uint8_t>& outBytes) {
    size_t total = bits.size();
    for (size_t i = 0; i + 7 < total; i += 8) {
        uint8_t b = 0;
        for (int bit = 0; bit < 8; bit++)
            b = (b << 1) | (bits[i + bit] & 1);
        outBytes.push_back(b);
    }
}

// ---- CRC32 ----
static const uint32_t crc_table[256] = {
    0x00000000L,0x77073096L,0xEE0E612CL,0x990951BAL, /* ... continuer la table 256 valeurs ... */
};

uint32_t Steg::ComputeCRC32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

// ---- EmbedMaster ----
bool Steg::EmbedMaster(LoadedImage& img, const std::string& msg) {
    if (img.pixels.empty()) return false;

    uint32_t length = static_cast<uint32_t>(msg.size());
    std::vector<uint8_t> payload;

    // MAGIC
    payload.push_back((MAGIC >> 24) & 0xFF);
    payload.push_back((MAGIC >> 16) & 0xFF);
    payload.push_back((MAGIC >> 8) & 0xFF);
    payload.push_back(MAGIC & 0xFF);

    // LENGTH
    payload.push_back((length >> 24) & 0xFF);
    payload.push_back((length >> 16) & 0xFF);
    payload.push_back((length >> 8) & 0xFF);
    payload.push_back(length & 0xFF);

    // CRC32
    uint32_t crc = ComputeCRC32((const uint8_t*)msg.data(), msg.size());
    payload.push_back((crc >> 24) & 0xFF);
    payload.push_back((crc >> 16) & 0xFF);
    payload.push_back((crc >> 8) & 0xFF);
    payload.push_back(crc & 0xFF);

    // MESSAGE
    for (char c : msg)
        payload.push_back(static_cast<uint8_t>(c));

    // Convert to bits
    std::vector<uint8_t> bits;
    BytesToBits(payload, bits);

    size_t totalBits = bits.size();
    size_t pixels = img.width * img.height;
    if (totalBits > pixels * 3) return false;

    size_t bitIndex = 0;
    for (size_t i = 0; i < pixels && bitIndex < totalBits; i++) {
        uint8_t* px = &img.pixels[i * 4];
        WriteBitLSB(px[0], bits[bitIndex++]);
        if (bitIndex >= totalBits) break;
        WriteBitLSB(px[1], bits[bitIndex++]);
        if (bitIndex >= totalBits) break;
        WriteBitLSB(px[2], bits[bitIndex++]);
    }

    return true;
}

// ---- ExtractMaster ----
bool Steg::ExtractMaster(const LoadedImage& img, std::string& outMsg) {
    if (img.pixels.empty()) return false;

    size_t pixels = img.width * img.height;
    std::vector<uint8_t> bits;
    bits.reserve(pixels * 3);

    for (size_t i = 0; i < pixels; i++) {
        const uint8_t* px = &img.pixels[i * 4];
        bits.push_back(ReadBitLSB(px[0]));
        bits.push_back(ReadBitLSB(px[1]));
        bits.push_back(ReadBitLSB(px[2]));
    }

    if (bits.size() < 96) return false; // Header + CRC

    std::vector<uint8_t> headerBytes;
    BitsToBytes(std::vector<uint8_t>(bits.begin(), bits.begin() + 96), headerBytes);

    uint32_t magic = (headerBytes[0] << 24) | (headerBytes[1] << 16) |
        (headerBytes[2] << 8) | headerBytes[3];
    if (magic != MAGIC) return false;

    uint32_t length = (headerBytes[4] << 24) | (headerBytes[5] << 16) |
        (headerBytes[6] << 8) | headerBytes[7];

    uint32_t crcStored = (headerBytes[8] << 24) | (headerBytes[9] << 16) |
        (headerBytes[10] << 8) | headerBytes[11];

    if (length == 0 || 12 + length > (bits.size() / 8)) return false;

    std::vector<uint8_t> msgBits(bits.begin() + 96, bits.begin() + 96 + length * 8);
    std::vector<uint8_t> msgBytes;
    BitsToBytes(msgBits, msgBytes);

    uint32_t crcCalc = ComputeCRC32(msgBytes.data(), msgBytes.size());
    if (crcCalc != crcStored) return false;

    outMsg.assign(msgBytes.begin(), msgBytes.end());
    return true;
}
