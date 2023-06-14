#pragma once
#include <cstdint>
#include <vector>
#include <map>

// Adapted from https://github.com/KyrietS/arithmetic-coding/blob/f80e7a911e898a45d114a1103149ad3b5feb0f40/AC_Core/src/AdaptiveScalingCoder.cpp

class Decoder {
private:
    const uint64_t powerOf(uint64_t a, uint64_t n)
    {
        return n == 0 ? 1 : a * powerOf(a, n - 1);
    }

    /* Parameters for encoding algorithm */
    const uint64_t PRECISION = 32;
    const uint64_t WHOLE = powerOf(2, PRECISION);
    const uint64_t HALF = WHOLE / 2;
    const uint64_t QUARTER = WHOLE / 4;
    unsigned int TOTAL_NUMBER_OF_SYMBOLS;
    std::vector<int> result;

public:
    std::vector<int>  decode(std::vector<uint8_t> bytes, std::map<int, int> uniqueVals, std::vector<float> accFreqs);
};
