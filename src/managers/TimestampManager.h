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
#ifndef PERFORMANCE_TEST

struct DeltaDeltaCompression {
    int timestampCurrent;
    int currentDelta;
    int previousDelta;
    int currentDeltaDelta;
    int timestampPrevious;
    bool readyForDelta;
    bool readyForDeltaDelta;
};
#endif
class TimestampManager {
public:
    int deltaDeltaTotalSize = 0;
    int firstTimestamp;

    struct Node* timestampCurrent;
    int totalFlushed;

    std::vector<std::pair<int, int>> offsetList;

    int currentOffset;

    std::unordered_map<int, std::vector<std::pair<int, int>>> localOffsetList;
    std::vector<int> localOffsetListToSend;
    std::vector<int> globalOffsetListToSend;

    std::unordered_map<int, std::vector<int>> deltaDeltas;
    std::unordered_map<int, BitVecBuilder> deltaDeltasBuilders;


    std::vector<std::function<void(BitVecBuilder *builder, int val)>> compressionSchemes;

    TimestampManager(ConfigManager &confMan, Timekeeper &timekeeper);

    void compressTimestamps(int timestamp);

    void makeLocalOffsetList(int lineNumber, int globalID);

    void deltaDeltaCompress(int lineNumber, int globalID);

    void finishDeltaDelta();

    void reconstructDeltaDelta();

    std::vector<int> flattenLOL();

    std::vector<int> flattenGOL();

    void
    getTimestampsByGlobalId(int globID, Node *timestampA,
                            Node *timestampB, std::vector<Node *> &res);

    bool decompressNextValue(std::vector<int> schemeVals, BitReader *bitReader, int* currentVal, std::vector<int> *decompressed, bool valueIsSigned);

    bool flushTimestamps(Node *lastUsedTimestamp);

    int flushLocalOffsetList(std::vector<std::pair<int, int>> &localOffsetListRef, int numberOfFlushedIndices);

    void finishAndResetDeltaDelta();
    size_t getSizeOfLocalOffsetList() const;
    size_t getSizeOfGlobalOffsetList() const;
    std::vector<DeltaDeltaCompression> latestTimestampsForDeltaDelta;

private:
    Timekeeper tk;
    int timestampPrevious;
    bool readyForOffset = false;
    Node *allTimestampsReconstructed;
    std::vector<TwoLatestTimestamps> latestTimestamps;
#ifndef PERFORMANCE_TEST

    std::tuple<int, int> deltaDeltaLimits(const int &val);

    std::unordered_map<int, int> deltaDeltaSizes;
#endif
    Timekeeper &timekeeper;

    void makeForwardListToSend(std::pair<int, int> &offset);

};