#pragma once

#include <vector>
#include <cstdint>
#include <map>
#include "../bitreader.h"

// Adapted from https://github.com/KyrietS/arithmetic-coding/blob/f80e7a911e898a45d114a1103149ad3b5feb0f40/AC_Core/src/AdaptiveScalingCoder.cpp

class Encoder {
private:
    BitVecBuilder out;

    const uint64_t powerOf(uint64_t a, uint64_t n)
    {
        return n == 0 ? 1 : a * powerOf(a, n - 1);
    }

    const uint64_t PRECISION = 32;
    const uint64_t WHOLE = powerOf(2, PRECISION);
    const uint64_t HALF = WHOLE / 2;
    const uint64_t QUARTER = WHOLE / 4;
    std::map<int, int> countFrequencies(const std::vector<int> &uncompressed);
public:
    Encoder();
    std::vector<uint8_t> encode(std::vector<int> in, std::map<int, int> *uniqueVals, std::vector<float> *accFreqs);
};

