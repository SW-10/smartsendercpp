#pragma once

#include "../constants.h"
#include "../utils/bitreader.h"
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

//struct BitVecBuilder {
//    uint8_t currentByte;
//    uint8_t remainingBits;
//    int bytesCounter;
//    std::vector<uint8_t> bytes;
//};

class Gorilla {
public:
    int lastTimestamp;
    uint8_t lastLeadingZeroBits;
    uint8_t lastTrailingZeroBits;
    BitVecBuilder compressedValues;
    size_t length;

    Gorilla();

    void fitValueGorilla(float value);

    [[nodiscard]] float getBytesPerValue() const;

    static std::vector<float>
    gridGorilla(std::vector<uint8_t> values, int valuesCount,
                int timestampCount);

private:
    float lastValue;

//    static void
//    appendBits(BitVecBuilder *data, long bits, uint8_t numberOfBits);

//    static void appendAZeroBit(BitVecBuilder *data);
//
//    static void appendAOneBit(BitVecBuilder *data);

    static size_t len(const BitVecBuilder &data);

    static uint8_t leadingZeros(const uint32_t &num);

    static uint8_t trailingZeros(const uint32_t &num);

    static uint32_t floatToBit(float val);

    std::vector<uint8_t> finish(BitVecBuilder *data);

    std::vector<uint8_t> finishWithOneBits(BitVecBuilder *data);
};
