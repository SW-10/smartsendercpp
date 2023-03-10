#include "Utils.h"

// Source: https://stackoverflow.com/questions/27431029/binary-search-with-returned-index-in-stl
// (Response by Jonathan L.)
int Utils::BinarySearch(std::vector<int> v, int data) {
    auto it = std::lower_bound(v.begin(), v.end(), data);
    if (it == v.end() || *it != data) {
        return -1;
    } else {
        std::size_t index = std::distance(v.begin(), it);
        return index;
    }
}

void Utils::binaryCompress(BitVecBuilder* builder, int val){
    if(val >= -63 && val <= 64){
        appendBits(builder, 0b10, 2);
        appendBits(builder, val, 7);
    } else if(val >= -255 && val <= 256){
        appendBits(builder, 0b110, 3);
        appendBits(builder, val, 9);
    } else if(val >= -2047 && val <= 2048){
        appendBits(builder, 0b1110, 4);
        appendBits(builder, val, 12);
    } else {
        appendBits(builder, 0b1111, 4);
        appendBits(builder, val, 32);
    }
}


std::vector<unsigned char> Utils::binaryCompressGlobOffsets
(std::vector<std::pair<int, int>> offsets){
    BitVecBuilder res;
    res.currentByte = 0;
    res.remainingBits = 8;
    res.bytesCounter = 0;
    for(const auto& elem : offsets){
        std::vector<int> temp;
        temp.push_back(elem.first);
        temp.push_back(elem.second);
        for(const auto& t : temp){
            Utils::binaryCompress(&res, t);
        }
    }
    return res.bytes;
}
std::vector<unsigned char>  Utils::binaryCompressLocOffsets(std::unordered_map<int,
        std::vector<std::pair<int, int>>>  offsets){

    BitVecBuilder res;
    res.currentByte = 0;
    res.remainingBits = 8;
    res.bytesCounter = 0;

    // Loop through all columns
    int globID = 0;
    for(const auto& list : offsets){

        // Compress global ID
        Utils::binaryCompress(&res, globID);

        //Loop through the offset list corresponding to current column
        for(const auto& elem : list.second ){
            std::vector<int> temp;
            temp.push_back(elem.first);
            temp.push_back(elem.second);
            for(const auto& t : temp){
                Utils::binaryCompress(&res, t);
            }
        }

        // Zero bit indicates end of column, i.e. the following compressed
        // number is the global id of the next column
        appendAZeroBit(&res);

        globID++;
    }

    return res.bytes;
}