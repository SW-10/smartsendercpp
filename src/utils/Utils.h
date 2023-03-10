#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>
#include "bitreader.h"

class Utils {
public:
    // Source: https://stackoverflow.com/questions/27431029/binary-search-with-returned-index-in-stl
    // (Response by Jonathan L.)
    static int BinarySearch(std::vector<int> v, int data);

    static void binaryCompress(BitVecBuilder* builder, int val);

    static std::vector<unsigned char> binaryCompressGlobOffsets(std::vector<std::pair<int, int>> offsets);

    static std::vector<unsigned char> binaryCompressLocOffsets(std::unordered_map<int,
            std::vector<std::pair<int, int>>>  offsets);

};

