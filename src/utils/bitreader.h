#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>

struct BitVecBuilder {
    uint8_t currentByte;
    uint8_t remainingBits;
    int bytesCounter;
    std::vector<uint8_t> bytes;

    BitVecBuilder(){
        currentByte = 0;
        remainingBits = 8;
        bytesCounter = 0;
    }
};

struct BitReader {
    BitReader(std::vector<uint8_t> bytes, int byteCount);
    size_t nextBit;
    int bytesCount;
    std::vector<uint8_t> bytes;
};

uint32_t readBits(BitReader *bitReader, uint8_t numberOfBits);

uint32_t readBit(BitReader *bitReader);

float intToFloat(uint32_t value);

void appendAZeroBit(BitVecBuilder *data);

void appendAOneBit(BitVecBuilder *data);

void appendBits(BitVecBuilder *data, long bits, uint8_t numberOfBits);