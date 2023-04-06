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

        DeltaDeltaCompression ddc = {0, 0, 0, 0, false, false};
        latestTimestampsForDeltaDelta.push_back(ddc);
    }
    makeCompressionSchemes();
    allTimestampsReconstructed = NULL;
    totalFlushed = 0;
}

void TimestampManager::compressTimestamps(int timestamp) {
//    timestampCount++;
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

std::vector<int> TimestampManager::reconstructTimestamps() {
    std::vector<int> reconstructed;
    reconstructed.push_back(firstTimestamp);
    int current = firstTimestamp;
    for (auto o: offsetList) {
        for (int i = 0; i < o.second; i++) {
            current += o.first;
            reconstructed.push_back(current);
        }
    }

    return reconstructed;
}

bool TimestampManager::calcIndexRangeFromTimestamps(int first, int second, int &first_out,
                                                    int &second_out) {
    int out1, out2;
    bool success = true;
    std::vector<int> reconstructed = reconstructTimestamps();
    out1 = Utils::BinarySearch(reconstructed, first);
    out2 = Utils::BinarySearch(reconstructed, second);

    if (out1 == -1) {
        std::cout << "First value (" << first << ")  in range not found";
        success = false;
    }
    if (out2 == -1) {
        std::cout << "Second value (" << second << ")  in range not found";
        success = false;
    }
    if (first >= second) {
        success = false;
    }

    first_out = out1;
    second_out = out2;

    return success;
}

int TimestampManager::getTimestampFromIndex(int index) {
    int count = 0;
    int current = firstTimestamp;
    for (auto o: offsetList) {
        for (int i = 0; i < o.second; i++) {
            if (count == index) {
                // Index found
                return current;
            }
            count++;
            current += o.first;
        }
    }

    // Index not found
    return -1;
}

std::vector<int>
TimestampManager::getTimestampsFromIndices(int index1, int index2) {
    std::vector<int> result;

    int count = 0;
    int current = firstTimestamp;
    for (auto o: offsetList) {
        for (int i = 0; i < o.second; i++) {
            if (count > index2) {
                return result;
            }
            if (count >= index1) {
                // First index found, add to 'result' until second index found
                result.push_back(current);
            }
            count++;
            current += o.first;
        }
    }
    // Index not found
    std::cout << "Timestamp range not valid";
    return result;
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
//    elem->timestampCurrent = lineNumber;

    if (!elem->readyForDelta){
        elem->timestampPrevious = lineNumber;
        std::cout << "ready delta" << elem->timestampPrevious << std::endl;
        elem->readyForDelta = true;

        deltaDeltas[globalID].push_back(lineNumber);

        auto a = deltaDeltaLimits(lineNumber);
        int ctrCode = std::get<0>(a);
        int numberOfBits = std::get<1>(a);
        uint8_t ctrCodeLength = getLengthOfBinaryRepresentation(ctrCode);

        appendBits(&deltaDeltasBuilders[globalID], ctrCode, ctrCodeLength);
        appendBits(&deltaDeltasBuilders[globalID], lineNumber, numberOfBits);


    } else if (elem->readyForDelta && !elem->readyForDeltaDelta) {

        elem->timestampCurrent = lineNumber;
        elem->currentDelta = lineNumber - elem->timestampPrevious/* something */;
        elem->readyForDeltaDelta = true;

        deltaDeltas[globalID].push_back(elem->currentDelta);

        auto a = deltaDeltaLimits(elem->currentDelta);
        int ctrCode = std::get<0>(a);
        int numberOfBits = std::get<1>(a);
        uint8_t ctrCodeLength = getLengthOfBinaryRepresentation(ctrCode);

        appendBits(&deltaDeltasBuilders[globalID], ctrCode, ctrCodeLength);
        appendBits(&deltaDeltasBuilders[globalID], elem->currentDelta, numberOfBits);

    } else if (elem->readyForDeltaDelta) {
        elem->timestampPrevious = elem->timestampCurrent;
        elem->timestampCurrent = lineNumber;
        elem->previousDelta = elem->currentDelta;
        elem->currentDelta = lineNumber - elem->timestampPrevious;
        elem->currentDeltaDelta = elem->previousDelta - elem->currentDelta;

        deltaDeltas[globalID].push_back(elem->currentDeltaDelta);

        auto a = deltaDeltaLimits(elem->currentDeltaDelta);
        int ctrCode = std::get<0>(a);
        int numberOfBits = std::get<1>(a);
        uint8_t ctrCodeLength = getLengthOfBinaryRepresentation(ctrCode);

        appendBits(&deltaDeltasBuilders[globalID], ctrCode, ctrCodeLength);
        appendBits(&deltaDeltasBuilders[globalID], elem->currentDeltaDelta, numberOfBits);

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
        return(std::tuple(0b11111, 32));
    }
}


void
TimestampManager::getTimestampsByGlobalId(int globID, Node *timestampA,
                                          Node *timestampB, std::vector<Node *> &res) {

    auto &localOffsets = localOffsetList.at(globID);
    bool found = false;
    //Create iterator for double linked list
    Node* iterator = timestampA;
    int count = 0;
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


void TimestampManager::makeCompressionSchemes() {
    // ===== SCHEME 1 =====:
    compressionSchemes.emplace_back([](BitVecBuilder *builder, int val) {
                                        if (val <= 3) {
                                            appendBits(builder, 0b10, 2);
                                            appendBits(builder, val, 2);
                                        } else if (val <= 15) {
                                            appendBits(builder, 0b110, 3);
                                            appendBits(builder, val, 4);
                                        } else if (val <= 63) {
                                            appendBits(builder, 0b1110, 4);
                                            appendBits(builder, val, 6);
                                        } else if (val <= 255) {
                                            appendBits(builder, 0b11110, 5);
                                            appendBits(builder, val, 8);
                                        } else if (val <= 1023) {
                                            appendBits(builder, 0b111110, 6);
                                            appendBits(builder, val, 10);
                                        } else if (val <= 4095) {
                                            appendBits(builder, 0b1111110, 7);
                                            appendBits(builder, val, 12);
                                        } else {
                                            appendBits(builder, 0b1111111, 7);
                                            appendBits(builder, val, 32);
                                        }
                                    }
    );

    // ===== SCHEME 2 =====:
    compressionSchemes.emplace_back([](BitVecBuilder *builder, int val) {
                                        if (val <= 127) {
                                            appendBits(builder, 0b10, 2);
                                            appendBits(builder, val, 7);
                                        } else if (val <= 511) {
                                            appendBits(builder, 0b110, 3);
                                            appendBits(builder, val, 9);
                                        } else if (val <= 4095) {
                                            appendBits(builder, 0b1110, 4);
                                            appendBits(builder, val, 12);
                                        } else {
                                            appendBits(builder, 0b1111, 4);
                                            appendBits(builder, val, 32);
                                        }
                                    }
    );

    // ===== SCHEME 3 =====:
    compressionSchemes.emplace_back([](BitVecBuilder *builder, int val) {
                                        if (val <= 4095) {
                                            appendBits(builder, 0b10, 2);
                                            appendBits(builder, val, 12);
                                        } else {
                                            appendBits(builder, 0b11, 2);
                                            appendBits(builder, val, 32);
                                        }
                                    }
    );

    // ===== SCHEME 4 =====:
    compressionSchemes.emplace_back([](BitVecBuilder *builder, int val) {
                                        if (val <= 3) {
                                            appendBits(builder, 0b10, 2);
                                            appendBits(builder, val, 2);
                                        } else if (val <= 7) {
                                            appendBits(builder, 0b110, 3);
                                            appendBits(builder, val, 3);
                                        } else if (val <= 15) {
                                            appendBits(builder, 0b1110, 4);
                                            appendBits(builder, val, 4);
                                        } else {
                                            appendBits(builder, 0b1111, 4);
                                            appendBits(builder, val, 32);
                                        }
                                    }
    );

    // =====================================================
    // =============== ADD MORE SCHEMES HERE ===============
    // =====================================================
}

std::vector<uint8_t>
TimestampManager::binaryCompressGlobOffsets(const std::vector<std::pair<int, int>> &offsets) {
    int size = 0;
    int bestSize = 0;
    int bestSchemeID = 0;

    // Loop through all compression schemes and pick the one that
    // gives the best compression
    std::vector<unsigned char> bestCompression;

    for (int schemeID = 0; schemeID < compressionSchemes.size(); schemeID++) {
        BitVecBuilder builder;

        const auto scheme = compressionSchemes.at(schemeID);

        // Compress scheme ID.
        // Always spend 4 bits on this value, as, when decompressing, we
        // don't have a compression scheme for the first value.
        // This allows for up to 256 different schemes.
        appendBits(&builder, schemeID, bitsUsedForSchemeID);

        for (const auto &elem: offsets) {
            scheme(&builder, elem.first);
            scheme(&builder, elem.second);

            // Stop trying if size becomes larger than what we have already stored
            if (builder.bytes.size() >= bestCompression.size() && !bestCompression.empty()) {
                break;
            }
        }

        // Update the chosen compression if the current scheme is better
        size = (builder.bytes.size() * 8) - builder.remainingBits;
        if (size < bestSize || bestCompression.empty()) {
            bestSize = size;
            bestCompression = std::move(builder.bytes);
            bestSchemeID = schemeID;
        }
    }

    // Get size and best scheme to represent size of the offset list
    size_t globSize = getSizeOfGlobalOffsetList();
    int bestSchemeForSize = findBestSchemeForSize(globSize);

    // The following code runs through everything again, uses the best compression schemes found
    // above and appends to the same builder.

    // Used for comparison between original and decompressed
    std::vector<int> originalFlat;

    BitVecBuilder finalCompression;

    auto schemeForSize = compressionSchemes.at(bestSchemeForSize);
    appendBits(&finalCompression, bestSchemeForSize, bitsUsedForSchemeID);
    schemeForSize(&finalCompression, globSize);
    originalFlat.emplace_back(globSize);


    appendBits(&finalCompression, bestSchemeID, bitsUsedForSchemeID);
    auto bestScheme = compressionSchemes.at(bestSchemeID);

    for (auto j: offsets) {
        bestScheme(&finalCompression, j.first);
        originalFlat.emplace_back(j.first);

        bestScheme(&finalCompression, j.second);
        originalFlat.emplace_back(j.second);
    }
    finalCompression.bytes.push_back(finalCompression.currentByte);
#ifndef NDEBUG
    // TEST
    // Check if all elements of the decompressed list are equivalent to the elements of the original
    // offset list.
    // Note: We compare to a flattened version of the original offset list to avoid formating the
    // decompressed list.
    {
        auto decompressed = decompressOffsetList(finalCompression.bytes);
        bool origEqDecompressed = true;
        for (int i = 0; i < originalFlat.size(); i++) {
            if (decompressed.at(i) != originalFlat.at(i)) {
                origEqDecompressed = false;
            }
        }
        CHECK(origEqDecompressed == true);
    }
#endif
    return finalCompression.bytes;
}

std::vector<uint8_t> TimestampManager::binaryCompressLocOffsets(
        std::unordered_map<int, std::vector<std::pair<int, int>>> offsets) {

    int size = 0;

    //Sort unordered map
    std::map<int, std::vector<std::pair<int, int>>> ordered(offsets.begin(),
                                                            offsets.end());

    std::vector<unsigned char> allColumnsCompressed;

    // Loop through all columns
    int globID = 0;

    std::vector<int> bestSchemes(ordered.size(), -1);
    int bestSize = 0;
    for (const auto &list: ordered) {

        std::vector<unsigned char> bestCompression;

        // Loop through all compression schemes and pick the one that
        // gives the best compression
        for (int schemeID = 0; schemeID < compressionSchemes.size(); schemeID++) {
            BitVecBuilder builder;

            const auto scheme = compressionSchemes.at(schemeID);

            // Compress scheme ID.
            // Always spend 4 bits on this value, as, when decompressing, we
            // don't have a compression scheme for the first value.
            // This allows for up to 256 different schemes.
            appendBits(&builder, schemeID, bitsUsedForSchemeID);

            // Compress index of first timestamp
            scheme(&builder, latestTimestamps.at(globID).timestampFirst);

            // Loop through the offset list corresponding to current column
            int count = 0;
            for (const auto &elem: list.second) {
                scheme(&builder, elem.first);
                scheme(&builder, elem.second);
                count++;
            }

            // Zero bit indicates end of column, i.e. the following compressed
            // number is the global id of the next column
            appendAZeroBit(&builder);

            // Update the chosen compression if the current scheme is better
            size = (builder.bytes.size() * 8) - builder.remainingBits;
            if (size < bestSize || bestCompression.empty()) {
                bestSchemes.at(globID) = schemeID;
                bestSize = size;
                bestCompression = builder.bytes;
            }
        }

        globID++;
    }

    // Get size and best scheme to represent size of the offset list
    size_t lolSize = getSizeOfLocalOffsetList();
    int bestSchemeForSize = findBestSchemeForSize(lolSize);

    // The following code runs through everything again, uses the best compression schemes found
    // above and appends to the same builder.

    // Used for comparison between original and decompressed
    std::vector<int> originalFlat;

    BitVecBuilder finalCompression;

    auto schemeForSize = compressionSchemes.at(bestSchemeForSize);
    appendBits(&finalCompression, bestSchemeForSize, bitsUsedForSchemeID);
    schemeForSize(&finalCompression, lolSize);
    originalFlat.emplace_back(lolSize);

    for (int i = 0; i < ordered.size(); i++) {

        auto scheme = compressionSchemes.at(bestSchemes.at(i));
        appendBits(&finalCompression, bestSchemes.at(i), bitsUsedForSchemeID);
        originalFlat.emplace_back(latestTimestamps.at(i).timestampFirst);
        scheme(&finalCompression, latestTimestamps.at(i).timestampFirst);

        for (auto j: ordered[i + 1]) {
            scheme(&finalCompression, j.first);
            originalFlat.emplace_back(j.first);

            scheme(&finalCompression, j.second);
            originalFlat.emplace_back(j.second);
        }
        appendAZeroBit(&finalCompression);
    }
    finalCompression.bytes.push_back(finalCompression.currentByte);

#ifndef NDEBUG
    // TEST
    // Check if all elements of the decompressed list are equivalent to the elements of the original
    // offset list.
    // Note: We compare to a flattened version of the original offset list to avoid formating the
    // decompressed list.
    {
        auto decompressed = decompressOffsetList(finalCompression.bytes);
        bool origEqDecompressed = true;
        for (int i = 0; i < originalFlat.size(); i++) {
            if (decompressed.at(i) != originalFlat.at(i)) {
                origEqDecompressed = false;
            }
        }
        CHECK(origEqDecompressed == true);
    }
#endif
    return finalCompression.bytes;
}

// Decompression function works for both global and local offset lists
std::vector<int>
TimestampManager::decompressOffsetList(const std::vector<uint8_t> &values) {

    std::map<int, std::vector<std::tuple<int, int>>> offsetsDecompressed;
    std::vector<int> temp;
    std::vector<int> decompressed;
    BitReader bitReader(values, values.size());

    std::vector<std::vector<int>> schemes;

    // Scheme 1
    schemes.push_back(std::vector<int>{2, 4, 6, 8, 10, 12, 32});

    // Scheme 2
    schemes.push_back(std::vector<int>{7, 9, 12, 32});

    // Scheme 3
    schemes.push_back(std::vector<int>{12, 32});

    // Scheme 4
    schemes.push_back(std::vector<int>{2, 3, 4, 32});

    // Scheme for size of local offset list
    int sizeSchemeID = readBits(&bitReader, bitsUsedForSchemeID);

    // Size of local offset list
    int lolSize = 0;
    decompressNextValue(schemes.at(sizeSchemeID), &bitReader, &lolSize, &decompressed);

    // Scheme for first column
    int currentValue = 0;
    int schemeID = readBits(&bitReader, bitsUsedForSchemeID);


    for (int i = 0; i < lolSize ; i++) {

        // decompressNextValue returns 'false' if no control codes are found, i.e. the zero bit at
        // the end of each column.
        // When that happens, read the next eight bits to receive new schemeID
        if (!decompressNextValue(schemes.at(schemeID), &bitReader, &currentValue, &decompressed)) {
            schemeID = readBits(&bitReader, bitsUsedForSchemeID);
        }
    }

    return decompressed;
}

bool
TimestampManager::decompressNextValue(std::vector<int> schemeVals, BitReader *bitReader,
                                      int *currentVal, std::vector<int> *decompressed) {
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
    *currentVal = readBits(bitReader, currentSchemeVal);
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

int TimestampManager::findBestSchemeForSize(int elements) {
    int bestSchemeForSize = 0;
    size_t bestSize = 0;
    std::vector<unsigned char> bestCompressionForSize;
    for (int schemeID = 0; schemeID < compressionSchemes.size(); schemeID++) {
        BitVecBuilder builder;

        const auto scheme = compressionSchemes.at(schemeID);

        // Compress scheme ID.
        // Always spend 4 bits on this value, as, when decompressing, we
        // don't have a compression scheme for the first value.
        // This allows for up to 256 different schemes.
        appendBits(&builder, schemeID, bitsUsedForSchemeID);

        // Compress size of global offset list
        scheme(&builder, elements);
        builder.bytes.push_back(builder.currentByte);

        // Update the chosen compression if the current scheme is better
        size_t size = (builder.bytes.size() * 8) - builder.remainingBits;

        if (size < bestSize || bestCompressionForSize.empty()) {
            bestSchemeForSize = schemeID;
            bestSize = (builder.bytes.size() * 8) - builder.remainingBits;
            bestCompressionForSize = builder.bytes;
        }
    }
    return bestSchemeForSize;
}

bool TimestampManager::flushTimestamps(
        Node *lastUsedTimestamp) {
    int index;
    allTimestampsReconstructed = lastUsedTimestamp;
    Node* iterator = lastUsedTimestamp->prev;
    if(iterator == NULL){
        return false;
    }
    for(index = 0; iterator->prev != NULL; index++){
        Node* temp = iterator->prev;
        delete iterator;
        iterator = temp;
    }
    delete iterator;
    index++;
    totalFlushed += index;
    allTimestampsReconstructed->prev = NULL;

    // Flush timestamps when last used timestamp is not the first in vector
    if (index != 0) {
        // Erase global timestamps

        // Deletion of local offset lists
        for (auto &lol: localOffsetList) {
            if (lol.second.empty()) continue;
            //Check whether the corresponding offset list contains deleted timestamps
            if(latestTimestamps.at(lol.first).timestampFirst < index){
                latestTimestamps.at(lol.first).timestampFirst = flushLocalOffsetList(lol.second, index-latestTimestamps.at(lol.first).timestampFirst);
            }
            else{
                latestTimestamps.at(lol.first).timestampFirst -= index;
            }
        }
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
            localOffsetListRef.erase(localOffsetListRef.begin());
        }
        else{
            offset = localOffsetListRef.front().first * instancesToFlush - numberOfFlushedIndices;
            localOffsetListRef.front().second -= instancesToFlush;
        }
    }
    else if (containedTimestamps == numberOfFlushedIndices) {
        localOffsetListRef.erase(localOffsetListRef.begin());
    }
    else{
        localOffsetListRef.erase(localOffsetListRef.begin());
        offset = flushLocalOffsetList(localOffsetListRef, numberOfFlushedIndices - containedTimestamps);
    }
    return offset;
}


std::vector<uint8_t> TimestampManager::binaryCompressLocOffsets2(
        std::unordered_map<int, std::vector<std::pair<int, int>>> offsets) {

    bool first = true;
    BitVecBuilder builder;

    std::vector<int> flatList;
    std::map<int, std::vector<int>> firsts;
    std::map<int, std::vector<int>> seconds;

    //Sort unordered map
    std::map<int, std::vector<std::pair<int, int>>> ordered(offsets.begin(),
                                                            offsets.end());


    int globID = 0;
    for(const auto i : ordered){
        flatList.emplace_back(i.first);
//        appendBits(&builder, i.first ,numberOfBits);
        for(auto j : i.second) {
            flatList.emplace_back(j.first);
            firsts[globID].emplace_back(j.first);
            flatList.emplace_back(j.second);
            seconds[globID].emplace_back(j.second);

//            appendBits(&builder, j.first ,numberOfBits);
//            appendBits(&builder, j.second ,numberOfBits);
        }
        globID++;
    }

    std::vector<int>::iterator it = flatList.begin();
    int ctrlCode = 0;
    int numberOfBits = 0;
    int upperLimit;
    if(first){
        auto bits = findNumberOfBits(*it);
        it++;
        ctrlCode        = std::get<0>(bits);
        numberOfBits    = std::get<1>(bits);
        upperLimit      = std::get<2>(bits);
    }

    std::cout << "FIRST: ctrl: " << ctrlCode << ", numberOfBits: " << numberOfBits << ", upperLimit: " << upperLimit << ", val: " << *it << std::endl;
    std::map<int, std::vector<BitVecBuilder>> firstCompressed;
    std::map<int, std::vector<BitVecBuilder>> secondsCompressed;

    unsigned bits, var = ctrlCode;
    for(bits = 0; var != 0; ++bits) var >>= 1;

    bool first2 = true;
    for(int i = 0; i < firsts.size(); i++){
        BitVecBuilder bvb;
        firstCompressed[i].emplace_back(bvb);
        for(const auto j : firsts.at(i)){
            if(first2){
                auto bits = findNumberOfBits(j);
                ctrlCode        = std::get<0>(bits);
                numberOfBits    = std::get<1>(bits);
                upperLimit      = std::get<2>(bits);
                first2 = false;
            }
            if(valueCanBeRepresented(upperLimit, j)){
                std::cout << "APPENDING BITS!\n" << j << std::endl;

                appendBits(&firstCompressed[i].back() , j, numberOfBits);
            } else {
                BitVecBuilder bvb;
                firstCompressed[i].emplace_back(bvb);
                auto bits = findNumberOfBits(j);
                std::cout << j <<  " CANT BE REPRESENTED WITH CONTROL CODE " << ctrlCode << ", LIMIT: " << upperLimit << std::endl;
                std::cout << "ADJUSTING UPPER LIMIT ..." << std::endl;
                ctrlCode        = std::get<0>(bits);
                numberOfBits    = std::get<1>(bits);
                upperLimit      = std::get<2>(bits);
                std::cout << "UPPER LIMIT ADJUSTED TO " << upperLimit << "\n" << std::endl;

                appendBits(&firstCompressed[i].back() , j, numberOfBits);
            }

        }
    }

    //Length of control bit
    // Src: https://stackoverflow.com/questions/29388711/how-to-get-the-bit-length-of-an-integer-in-c
//    unsigned bits, var = ctrlCode;
//    for(bits = 0; var != 0; ++bits) var >>= 1;
//
//    appendBits(&builder, ctrlCode, bits);
//
//    for(; it != flatList.end(); ++it){
//        if(valueCanBeRepresented(upperLimit, *it)) {
//            std::cout << "APPENDING BIT :D" << std::endl;
//            appendBits(&builder, *it, numberOfBits);
//        }
//        else {
//            std::cout << *it <<  " CANT BE REPRESENTED WITH CONTROL CODE " << ctrlCode << ", LIMIT: " << upperLimit << std::endl;
//            std::cout << "ADJUSTING UPPER LIMIT ..." << std::endl;
//            auto bits = findNumberOfBits(*it);
//            ctrlCode        = std::get<0>(bits);
//            numberOfBits    = std::get<1>(bits);
//            upperLimit      = std::get<2>(bits);
//            std::cout << "UPPER LIMIT ADJUSTED TO " << upperLimit << "\n" << std::endl;
//            appendBits(&builder, *it, numberOfBits);
//        }
//    }

    std::vector<uint8_t> res;
    return res;
}

bool TimestampManager::valueCanBeRepresented(const int &currentLimit,  const int &val){
    // Extract the optimal upper limit for the value
    auto hej = findNumberOfBits(val);
    int bestLimit = std::get<2>(hej);
    return val <= currentLimit && currentLimit == bestLimit;
}

std::tuple<int, int, int> TimestampManager::findNumberOfBits(const int &val){
    if (val <= 1) {
        return(std::tuple(0b10, 1, 1));
    } else if (val <= 3) {
        return(std::tuple(0b110, 2, 3));
    } else if (val <= 7) {
        return(std::tuple(0b1110, 3, 7));
    } else if (val <= 15) {
        return(std::tuple(0b11110, 4, 15));
    } else if (val <= 31){
        return(std::tuple(0b111110, 5, 31));
    } else if (val <= 63){
        return(std::tuple(0b1111110, 6, 63));
    } else if (val <= 127){
        return(std::tuple(0b11111110, 7, 127));
    } else if (val <= 255){
        return(std::tuple(0b111111110, 8, 255));
    } else if (val <= 511){
        return(std::tuple(0b1111111110, 9, 511));

        // TODO: Add more cases here

    } else {
        return(std::tuple(0b1111111111, 32, INT32_MAX));
    }
}

#pragma clang diagnostic pop

