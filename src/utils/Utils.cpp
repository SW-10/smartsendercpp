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