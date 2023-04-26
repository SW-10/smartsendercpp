#include "../utils/Utils.h"
#include "../constants.h"
#include "TimestampManager.h"
#include <algorithm>
#include <iostream>
#include <tuple>
#ifndef NDEBUG
#include "../doctest.h"
#endif


#include "ReaderManager.h"
//
//extern Timekeeper *timekeeper;

TimestampManager::TimestampManager(ConfigManager &confMan, Timekeeper &timekeeper) : timekeeper(timekeeper){
    for (int i = 0; i < confMan.totalNumberOfCols; i++) {
        TwoLatestTimestamps ts = {0, 0, false};
        latestTimestamps.push_back(ts);

        DeltaDeltaCompression ddc = {0, 0, 0, 0, 0, false, false};
        latestTimestampsForDeltaDelta.push_back(ddc);
    }

    allTimestampsReconstructed = NULL;
    totalFlushed = 0;
}

void TimestampManager::compressTimestamps(int timestamp) {
    timekeeper.update(timestamp);

    if (!readyForOffset){
        allTimestampsReconstructed = Utils::insert_end(&allTimestampsReconstructed, timestamp);
        timestampCurrent = allTimestampsReconstructed;
        firstTimestamp = timestamp;
    }
    if (readyForOffset) {
        timestampCurrent = Utils::insert_end(&timestampCurrent, timestamp);
        // Offset = Difference between current and previous timestamp
        currentOffset = timestamp - timestampPrevious;

        // Insert new offset if first element or if current offset is not equal to previous offset
        // Else, increase counter for current offset
        if (offsetList.empty() ||
            currentOffset != offsetList.at(offsetList.size() - 1).first) {
            offsetList.emplace_back(currentOffset, 1);
        } else {
            offsetList.at(offsetList.size() - 1).second++;
        }
    }

    timestampPrevious = timestamp;
    readyForOffset = true;
}


void TimestampManager::makeLocalOffsetList(int lineNumber, int globalID) {
    auto elem = &latestTimestamps.at(globalID);
    elem->timestampCurrent = lineNumber;

    if (!elem->readyForOffset) elem->timestampFirst = lineNumber;
    if (elem->readyForOffset) {

        // Offset = Difference between current and previous timestamp
        elem->currentOffset = elem->timestampCurrent - elem->timestampPrevious;


        // Insert new offset if first element or if current offset is not equal to previous offset
        // Else, increase counter for current offset
        if (localOffsetList[globalID].empty() || elem->currentOffset != localOffsetList[globalID][
                localOffsetList[globalID].size() - 1].first) {
            localOffsetList[globalID].emplace_back(elem->currentOffset, 1);
        } else {

            localOffsetList[globalID][localOffsetList[globalID].size() - 1].second++;
        }
    }

    elem->timestampPrevious = elem->timestampCurrent;
    elem->readyForOffset = true;
}

void TimestampManager::deltaDeltaCompress(int lineNumber, int globalID) {
    auto elem = &latestTimestampsForDeltaDelta.at(globalID);
    if(!deltaDeltaSizes.contains(globalID)){
        deltaDeltaSizes[globalID] = 0;
    }

    if (!elem->readyForDelta){
        elem->timestampPrevious = lineNumber;
        elem->readyForDelta = true;

        deltaDeltas[globalID].push_back(lineNumber);

        auto a = deltaDeltaLimits(lineNumber);
        int ctrCode = std::get<0>(a);
        int numberOfBits = std::get<1>(a);
        uint8_t ctrCodeLength = getLengthOfBinaryRepresentation(ctrCode);

        deltaDeltaSizes[globalID]++;
        appendBits(&deltaDeltasBuilders[globalID], ctrCode, ctrCodeLength);
        appendBits(&deltaDeltasBuilders[globalID], lineNumber, numberOfBits);


    } else if (elem->readyForDelta && !elem->readyForDeltaDelta) {

        elem->timestampCurrent = lineNumber;
        elem->currentDelta = lineNumber - elem->timestampPrevious;
        elem->readyForDeltaDelta = true;

        deltaDeltas[globalID].push_back(elem->currentDelta);

        auto a = deltaDeltaLimits(elem->currentDelta);
        int ctrCode = std::get<0>(a);
        int numberOfBits = std::get<1>(a);
        uint8_t ctrCodeLength = getLengthOfBinaryRepresentation(ctrCode);

        deltaDeltaSizes[globalID]++;
        appendBits(&deltaDeltasBuilders[globalID], ctrCode, ctrCodeLength);
        appendBits(&deltaDeltasBuilders[globalID], elem->currentDelta, numberOfBits);

    } else if (elem->readyForDeltaDelta) {
        elem->timestampPrevious = elem->timestampCurrent;
        elem->timestampCurrent = lineNumber;
        elem->previousDelta = elem->currentDelta;
        elem->currentDelta = lineNumber - elem->timestampPrevious;
//        elem->currentDeltaDelta = elem->previousDelta - elem->currentDelta;
        elem->currentDeltaDelta = elem->currentDelta - elem->previousDelta ;

        deltaDeltas[globalID].push_back(elem->currentDeltaDelta);

        auto a = deltaDeltaLimits(elem->currentDeltaDelta);
        int ctrCode = std::get<0>(a);
        int numberOfBits = std::get<1>(a);
        uint8_t ctrCodeLength = getLengthOfBinaryRepresentation(ctrCode);

        deltaDeltaSizes[globalID]++;
        appendBits(&deltaDeltasBuilders[globalID], ctrCode, ctrCodeLength);
        appendBits(&deltaDeltasBuilders[globalID], elem->currentDeltaDelta, numberOfBits);

    }
}

void TimestampManager::finishDeltaDelta(){
    int bytes = 0;
    for(auto &elem : deltaDeltasBuilders){
        elem.second.bytes.emplace_back(elem.second.currentByte);
        bytes += elem.second.bytes.size();
    }
    std::cout << "Delta delta size: " << bytes << " bytes!" << std::endl;
}

void TimestampManager::reconstructDeltaDelta(){
    std::map<int, std::vector<int>> reconstructed;
    std::vector<int> schemeVals = {7, 9, 12, 32};
    for(int globID = 1; globID <= deltaDeltasBuilders.size(); globID++){
        BitReader br(deltaDeltasBuilders[globID].bytes, deltaDeltasBuilders[globID].bytes.size());
        std::vector<int> decompressed;

        for(int i = 0; i < deltaDeltaSizes[globID]; i++){
            int currentVal = 0;

            if(!decompressNextValue(schemeVals, &br, &currentVal, &reconstructed[globID], true)){
                currentVal = 0;
                reconstructed[globID].emplace_back(currentVal);
            }
        }
    }
}



std::tuple<int, int> TimestampManager::deltaDeltaLimits(const int &val){
    if (val == 0) {
        return(std::tuple(0b0, 0));
    } else if (val >= -64 && val <= 63){
        return(std::tuple(0b10, 7));
    } else if (val >= -256 && val <= 255) {
        return (std::tuple(0b110, 9));
    } else if (val >= -2048 && val <= 2047) {
        return (std::tuple(0b1110, 12));
    } else {
        return(std::tuple(0b1111, 32));
    }
}

std::vector<int> TimestampManager::flattenLOL(){
    std::vector<int> flat;
    //Sort unordered map
    std::map<int, std::vector<std::pair<int, int>>> ordered(localOffsetList.begin(),
                                                            localOffsetList.end());
    int count = 0;
    for(const auto &column : ordered){
        flat.emplace_back(latestTimestamps.at(count).timestampFirst);
        for(const auto &row : column.second){
            flat.emplace_back(row.first);
            flat.emplace_back(row.second);
        }
        count++;
        // -2 is a value representing end of column
        flat.emplace_back(-2);
    }
    // -3 is a value representing end of list
    flat.back() = -3;
    return flat;
}

std::vector<int> TimestampManager::flattenGOL(){
    std::vector<int> flat;
    for(const auto &row : offsetList){
        flat.emplace_back(row.first);
        flat.emplace_back(row.second);
    }
    // -3 is a value representing end of list
    flat.emplace_back(-3);
    return flat;
}


void
TimestampManager::getTimestampsByGlobalId(int globID, Node *timestampA,
                                          Node *timestampB, std::vector<Node *> &res) {

    auto &localOffsets = localOffsetList.at(globID);
    bool found = false;
    //Create iterator for double linked list
    Node* iterator = timestampA;
    int count = latestTimestamps[globID].timestampFirst;
    for (auto & localOffset : localOffsets){
        count += localOffset.first*localOffset.second;
        if(res.empty() && count+totalFlushed > timestampA->index){
            //Define local offset start by current timestamp
            int start = (timestampA->index-(count+totalFlushed-localOffset.first*localOffset.second))/localOffset.first;
            for (int j = start; j < localOffset.second; j++){
                //Forward offset size in iterator
                iterator = Utils::forwardNode(iterator, localOffset.first);
                res.emplace_back(iterator);
                //Break when timestampB is found
                if(iterator == timestampB) {
                    found = true;
                    break;
                }
            }
        }
        else if(!res.empty()){
            for (int j = 0; j < localOffset.second; j++){
                iterator = Utils::forwardNode(iterator, localOffset.first);
                res.emplace_back(iterator);
                if(iterator == timestampB) {
                    found = true;
                    break;
                }
            }
        }

        if(found){
            break;
        }
    }
    // TODO: RUN IF DEBUG / make runtime test
    if(!found){
        std::cout << "getTimestampsByGlobalId failed" << std::endl;
    }
}

bool
TimestampManager::decompressNextValue(std::vector<int> schemeVals, BitReader *bitReader,
                                      int *currentVal, std::vector<int> *decompressed,
                                      bool valueIsSigned) {
    int currentSchemeVal = 0;

    // Read the control bits. The last control code of all schemes have the same length as the
    // second to last code, but the last bit is '1' instead of '0'.
    // Condition #1 ensures that we correctly read the last control code correctly.
    //      This is the first condition, as it short-circuits if false (which is intentional)
    // Condition #2 reads the bits and breaks out of the loop when a 0-bit is encountered in
    //      the control bit. The 0-bit is also consumed.
    int i = 0;
    for (; i < schemeVals.size() && readBit(bitReader); i++) { /* Empty body is intentional */ }

    if (i == 0) { // readBit returned 0, i.e. we didn't find any control codes. End of column!
        return false;
    } else { // Found control code
        currentSchemeVal = schemeVals.at(i - 1);
    }

    // Read number of bits corresponding to the found control bit
    if(valueIsSigned){
        *currentVal = readBitsSigned(bitReader, currentSchemeVal);
    } else {
        *currentVal = readBits(bitReader, currentSchemeVal);
    }
//    std::cout << "CURRENT VAL: " << *currentVal << " bits to read: " << currentSchemeVal << std::endl;
    decompressed->emplace_back(*currentVal);

    return true;
}

size_t TimestampManager::getSizeOfLocalOffsetList() const {
    int size = 0;

    // Offsets
    for (const auto &i: localOffsetList) {
        size+=2;    // Each pair consists of two elements:
                    //  1: Scheme ID
                    //  2: First timestamp
        for (const auto &j: i.second) {
            size += 2; // Each element in localOffsetList consists of two numbers (a pair)
        }
    }
    return size;
}

size_t TimestampManager::getSizeOfGlobalOffsetList() const {
    return offsetList.size()*2;
}


bool TimestampManager::flushTimestamps(
        Node *lastUsedTimestamp) {
    int index;
    allTimestampsReconstructed = lastUsedTimestamp;
    Node* iterator = lastUsedTimestamp->prev;
    if(iterator == NULL){
        return false;
    } else{
        iterator = iterator -> prev;
        if(iterator == NULL){
            return false;
        }
    }
    int currentOffset_ = 0;
    for(index = 0; iterator->prev != NULL; index++){
        Node* temp = iterator->prev;
        if (iterator->data - temp->data != currentOffset_){
            currentOffset_ = iterator->data - temp->data;
            globalOffsetListToSend.emplace_back(currentOffset_);
            globalOffsetListToSend.emplace_back(1);
        }
        else{
            globalOffsetListToSend.at(globalOffsetListToSend.size()-1)++;
        }
        delete iterator;
        iterator = temp;
    }
    delete iterator;
    index++;
    totalFlushed += index;
    allTimestampsReconstructed->prev = NULL;

    // Flush timestamps when last used timestamp is not the first in vector
    if (index != 0) {
        globalOffsetListToSend.emplace_back(-3);
        // Erase global timestamps

        // Deletion of local offset lists
        //for (auto &lol: localOffsetList) {
        for (int i = 0; i<localOffsetList.size(); i++){
            auto &lol = localOffsetList[i];
            if (lol.empty()) continue;
            //Check whether the corresponding offset list contains deleted timestamps
            if(latestTimestamps.at(i).timestampFirst < index){
                latestTimestamps.at(i).timestampFirst = flushLocalOffsetList(lol, index-latestTimestamps.at(i).timestampFirst);
            }
            else{
                latestTimestamps.at(i).timestampFirst -= index;
            }
            if(i != localOffsetList.size()-1){
                localOffsetListToSend.emplace_back(-2);
            }
        }
        localOffsetListToSend.emplace_back(-3);
        return true;
    }
    return false;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
int TimestampManager::flushLocalOffsetList(std::vector<std::pair<int, int>> &localOffsetListRef, int numberOfFlushedIndices){
    int offset = 0;
    // Calculate number of timestamps in first offset
    int containedTimestamps = localOffsetListRef.front().first * localOffsetListRef.front().second;
    if (containedTimestamps > numberOfFlushedIndices){
        // How many instances can be flushed
        int instancesToFlush = numberOfFlushedIndices / localOffsetListRef.front().first;
        // If flushables is not even, then add one to flush
        if(numberOfFlushedIndices % localOffsetListRef.front().first > 0){
            instancesToFlush += 1;
        }
        if (instancesToFlush == localOffsetListRef.front().second){
            offset = localOffsetListRef.front().first * instancesToFlush - numberOfFlushedIndices;
            makeForwardListToSend(localOffsetListRef.at(0));
            localOffsetListRef.erase(localOffsetListRef.begin());
        }
        else{
            offset = localOffsetListRef.front().first * instancesToFlush - numberOfFlushedIndices;
            auto toSave = std::make_pair(localOffsetListRef.at(0).first,instancesToFlush);
            localOffsetListRef.front().second -= instancesToFlush;
            makeForwardListToSend(toSave);
        }
    }
    else if (containedTimestamps == numberOfFlushedIndices) {
        makeForwardListToSend(localOffsetListRef.at(0));
        localOffsetListRef.erase(localOffsetListRef.begin());
    }
    else{
        makeForwardListToSend(localOffsetListRef.at(0));
        localOffsetListRef.erase(localOffsetListRef.begin());
        offset = flushLocalOffsetList(localOffsetListRef, numberOfFlushedIndices - containedTimestamps);
    }
    return offset;
}

void TimestampManager::makeForwardListToSend(std::pair<int, int> &offset){
    localOffsetListToSend.emplace_back(offset.first);
    localOffsetListToSend.emplace_back(offset.second);
}

#pragma clang diagnostic pop

