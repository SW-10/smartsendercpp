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

class TimestampManager{
private:
    std::vector<int> allTimestampsReconstructed;
    int firstTimestamp;
    int timestampCurrent;
    int timestampPrevious;
    bool readyForOffset = false;

    std::vector<TwoLatestTimestamps> latestTimestamps;

    std::vector<std::pair<int, int>> offsetList;
    std::unordered_map<int, std::vector<std::pair<int, int>>> localOffsetList;

    int currentOffset;
    std::map<int, int> offsets;
public:
    TimestampManager(ConfigManager &confMan);
    std::vector<std::pair<int, int>> getOffsetList(){ return offsetList; }
    int getFirstTimestamp(){ return firstTimestamp; }
    void compressTimestamps(int timestamp);
    std::vector<int> reconstructTimestamps();
    bool calcIndexRangeFromTimestamps(int first, int second, int& first_out, int& second_out);
    int getTimestampFromIndex(int index);
    std::vector<int> getTimestampsFromIndices(int index1, int index2);

    void makeLocalOffsetList(int lineNumber, int globalID);
    std::vector<int> getTimestampRangeForColumns(int globID, int indexA, int indexB);
    int getTimestampsFromIndexForColumns(int globID, int index);
    int getCurrentTimestamp(){return timestampCurrent;}
    std::vector<int> reconstructNTimestamps(int n);
};