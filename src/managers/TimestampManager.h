#pragma once

#include <vector>
#include <utility>
#include <map>
#include <unordered_map>
#include "ConfigManager.h"

struct TwoLatestTimestamps {
    int timestampCurrent;
    int timestampPrevious;
    int timestampFirst;
    int currentOffset;
    bool readyForOffset;
};

class TimestampManager {
public:
    int firstTimestamp;

    int timestampCurrent;

    std::vector<std::pair<int, int>> offsetList;

    int currentOffset;

    std::map<int, int> offsets;

    std::unordered_map<int, std::vector<std::pair<int, int>>> localOffsetList;

    TimestampManager(ConfigManager &confMan);

    void compressTimestamps(int timestamp);

    std::vector<int> reconstructTimestamps();

    bool calcIndexRangeFromTimestamps(int first, int second, int &first_out,
                                      int &second_out);

    int getTimestampFromIndex(int index);

    std::vector<int> getTimestampsFromIndices(int index1, int index2);

    void makeLocalOffsetList(int lineNumber, int globalID);

    std::vector<int>
    getTimestampRangeForColumns(int globID, int indexA, int indexB);

    std::vector<int>
    getTimestampsByGlobalId(int globID, int timestampA, int timestampB);

    int getTimestampsFromIndexForColumns(int globID, int index);

    std::vector<int> reconstructNTimestamps(int n);

private:
    int timestampPrevious;
    bool readyForOffset = false;
    std::vector<int> allTimestampsReconstructed;
    std::vector<TwoLatestTimestamps> latestTimestamps;
};