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

/**
 * @param globOffsetList The global offset list
 * @param offsetLists The map of local offset lists
 * @return The number of values stored within the two input lists in total
 */
// Inputs the global offset list and the local offset lists and
int Utils::getSizeOfOffsetLists(std::vector<std::pair<int, int>> globOffsetList, std::unordered_map<int,std::vector<std::pair<int, int>>> offsetLists){
    int res = 0;
    for(auto& g : globOffsetList){
        res += 2;
    }

    for(auto& o : offsetLists){
        for(auto& loc : o.second){
            res += 2;
        }
    }
    return res;
}