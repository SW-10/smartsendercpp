#pragma once

#include "../constants.h"
#include "../utils/bitreader.h"
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../utils/Utils.h"


class Gorilla {
public:
    Node* lastTimestamp;
    uint8_t lastLeadingZeroBits;
    uint8_t lastTrailingZeroBits;
    BitVecBuilder compressedValues;
    size_t length;

    Gorilla();

    bool fitValueGorilla(float value);

    [[nodiscard]] float getBytesPerValue() const;

    static std::vector<float>
    gridGorilla(std::vector<uint8_t> values, int valuesCount,
                int timestampCount);

    static std::vector<uint8_t> getNFirstValuesBitstring(int N, std::vector<uint8_t> bytes);

private:
    float lastValue;

    static size_t len(const BitVecBuilder &data);

    static uint8_t leadingZeros(const uint32_t &num);

    static uint8_t trailingZeros(const uint32_t &num);

    static uint32_t floatToBit(float val);

    std::vector<uint8_t> finish(BitVecBuilder *data);

    std::vector<uint8_t> finishWithOneBits(BitVecBuilder *data);
};
