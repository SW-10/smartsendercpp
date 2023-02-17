#ifndef BITREADER
#define BITREADER
#include <stdint.h>
#include <stdlib.h>
#include <vector>

struct BitReader{
    size_t nextBit;
    int bytesCount;
    std::vector<uint8_t> bytes;
}typedef BitReader;
