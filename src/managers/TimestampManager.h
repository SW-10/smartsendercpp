#pragma once

#include <vector>
#include <utility>
#include <map>
#include <unordered_map>
#include <functional>
#include "ConfigManager.h"
#include "../utils/Utils.h"
#include "../utils/Timekeeper.h"


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

    std::unordered_map<int, std::vector<std::pair<int, int>>> localOffsetList;

    std::vector<std::function<void(BitVecBuilder *builder, int val)>> compressionSchemes;

    TimestampManager(ConfigManager &confMan, Timekeeper &timekeeper);

    void compressTimestamps(int timestamp);

    std::vector<int> reconstructTimestamps();

    bool calcIndexRangeFromTimestamps(int first, int second, int &first_out, int &second_out);

    int getTimestampFromIndex(int index);

    std::vector<int> getTimestampsFromIndices(int index1, int index2);

    void makeLocalOffsetList(int lineNumber, int globalID);

    std::vector<int>
    getTimestampRangeForColumns(int globID, int indexA, int indexB);

    std::vector<int>
    getTimestampsByGlobalId(int globID, int timestampA, int timestampB);

    int getTimestampsFromIndexForColumns(int globID, int index);

    std::vector<int> reconstructNTimestamps(int n);

    void makeCompressionSchemes();

    std::vector<uint8_t> binaryCompressGlobOffsets(const std::vector<std::pair<int, int>> &offsets);

    std::vector<uint8_t>
    binaryCompressLocOffsets(std::unordered_map<int, std::vector<std::pair<int, int>>> offsets);

    std::vector<int> decompressOffsetList(const std::vector<uint8_t> &values);
    bool decompressNextValue(std::vector<int> schemeVals, BitReader *bitReader, int* currentVal, std::vector<int> *decompressed);

    bool flushTimestamps(int lastUsedTimestamp);
    TimestampManager();
    static int flushLocalOffsetList(std::vector<std::pair<int, int>> &localOffsetListRef, int numberOfFlushedIndices);
    size_t getSizeOfLocalOffsetList() const;
    size_t getSizeOfGlobalOffsetList() const;
private:
    Timekeeper tk;
    const int bitsUsedForSchemeID = 4 ;
    int timestampPrevious;
    bool readyForOffset = false;
    std::vector<int> allTimestampsReconstructed;
    std::vector<TwoLatestTimestamps> latestTimestamps;
    int findBestSchemeForSize(int elements);

    Timekeeper &timekeeper;
};