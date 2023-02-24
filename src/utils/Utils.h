#pragma once

#include <vector>

class Utils {
public:
    // Source: https://stackoverflow.com/questions/27431029/binary-search-with-returned-index-in-stl
    // (Response by Jonathan L.)
    static int BinarySearch(std::vector<int> v, int data);
};

