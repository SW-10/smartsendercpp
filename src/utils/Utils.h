#pragma once

#include <vector>
#include <unordered_map>


class Utils {
public:
    // Source: https://stackoverflow.com/questions/27431029/binary-search-with-returned-index-in-stl
    // (Response by Jonathan L.)
    static int BinarySearch(std::vector<int> v, int data);
    static int getSizeOfOffsetLists(std::vector<std::pair<int, int>> globOffsetList, std::unordered_map<int,std::vector<std::pair<int, int>>> LocalOffsetLists);
};

