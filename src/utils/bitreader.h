#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>

struct BitReader {
    size_t nextBit;
    int bytesCount;
    std::vector<uint8_t> bytes;
};

BitReader tryNewBitReader(std::vector<uint8_t> bytes, int byteCount);

uint32_t readBits(BitReader *bitReader, uint8_t numberOfBits);

uint32_t readBit(BitReader *bitReader);

float intToFloat(uint32_t value);