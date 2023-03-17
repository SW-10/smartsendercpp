#include "../utils/Utils.h"
#include "TimestampManager.h"
#include <iostream>
#include "../constants.h"
#include <tuple>


TimestampManager::TimestampManager(ConfigManager &confMan) {
    for (int i = 0; i < confMan.totalNumberOfCols; i++) {
        TwoLatestTimestamps ts = {0, 0, false};
        latestTimestamps.push_back(ts);
    }
    makeCompressionSchemes();
}

void TimestampManager::compressTimestamps(int timestamp) {
//    timestampCount++;
    allTimestampsReconstructed.push_back(timestamp);

    timestampCurrent = timestamp;
    if (!readyForOffset) firstTimestamp = timestamp;
    if (readyForOffset) {

        // Offset = Difference between current and previous timestamp
        currentOffset = timestampCurrent - timestampPrevious;

        // Insert new offset if first element or if current offset is not equal to previous offset
        // Else, increase counter for current offset
        if (offsetList.empty() ||
            currentOffset != offsetList[offsetList.size() - 1].first) {
            offsetList.emplace_back(currentOffset, 1);
        } else {
            offsetList[offsetList.size() - 1].second++;
        }
    }

    timestampPrevious = timestampCurrent;
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

std::vector<int>
TimestampManager::getTimestampsByGlobalId(int globID, int timestampA, int timestampB) {
    auto localOffsets = localOffsetList[globID];
    auto firstLocalTimestamp = latestTimestamps[globID].timestampFirst;
    std::vector<int> res;

    int count = firstLocalTimestamp;

    for (auto &localOffset: localOffsets) {
        int firstTimeOffset = static_cast<int>(count == firstLocalTimestamp);

        for (int j = 0; j < localOffset.second; j++) {
            if (allTimestampsReconstructed.at(count) > timestampA) {
                if (allTimestampsReconstructed.at(count) > timestampB) {
                    break;
                }

                res.push_back(allTimestampsReconstructed.at(count));
            }
            count += localOffset.first;
        }
    }
    res.push_back(allTimestampsReconstructed.at(count)); // Add last time stamp

    return res;
}


void TimestampManager::makeCompressionSchemes() {
    // ===== SCHEME 1 =====:
    compressionSchemes.emplace_back([](BitVecBuilder *builder, int val) {
                                        if (val >= -2 && val <= 3) {
                                            appendBits(builder, 0b10, 2);
                                            appendBits(builder, val, 3);
                                        } else if (val >= -14 && val <= 15) {
                                            appendBits(builder, 0b110, 3);
                                            appendBits(builder, val, 5);
                                        } else if (val >= -63 && val <= 64) {
                                            appendBits(builder, 0b1110, 4);
                                            appendBits(builder, val, 7);
                                        } else if (val >= -255 && val <= 256) {
                                            appendBits(builder, 0b11110, 5);
                                            appendBits(builder, val, 9);
                                        } else if (val >= -1023 && val <= 1024) {
                                            appendBits(builder, 0b111110, 6);
                                            appendBits(builder, val, 11);
                                        } else if (val >= -4095 && val <= 4096) {
                                            appendBits(builder, 0b1111110, 7);
                                            appendBits(builder, val, 13);
                                        } else {
                                            appendBits(builder, 0b1111111, 7);
                                            appendBits(builder, val, 32);
                                        }
                                    }
    );

    // ===== SCHEME 2 =====:
    compressionSchemes.emplace_back([](BitVecBuilder *builder, int val) {
                                        if (val >= -63 && val <= 64) {
                                            appendBits(builder, 0b10, 2);
                                            appendBits(builder, val, 7);
                                        } else if (val >= -255 && val <= 256) {
                                            appendBits(builder, 0b110, 3);
                                            appendBits(builder, val, 9);
                                        } else if (val >= -2047 && val <= 2048) {
                                            appendBits(builder, 0b1110, 4);
                                            appendBits(builder, val, 12);
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
    std::vector<unsigned char> bestCompression;
    for (int schemeID = 0; schemeID < compressionSchemes.size(); schemeID++) {
        BitVecBuilder builder;
        builder.currentByte = 0;
        builder.remainingBits = 8;
        builder.bytesCounter = 0;
        const auto scheme = compressionSchemes.at(schemeID);

        // Compress scheme ID.
        // Always spend 8 bits on this value, as, when decompressing, we
        // don't have a compression scheme for the first value.
        // This allows for up to 256 different schemes.
        appendBits(&builder, schemeID, 8);

        for (const auto &elem: offsets) {
            scheme(&builder, elem.first);
            scheme(&builder, elem.second);

            // Stop trying if size becomes larger than what we have already stored
            if (builder.bytes.size() >= bestCompression.size() && !bestCompression.empty()) {
                break;
            }
        }

        // Update the chosen compression if the current scheme is better
        size = builder.bytes.size();
        if (size < bestCompression.size() || bestCompression.empty()) {
            bestCompression = std::move(builder.bytes);
        }
    }
    return bestCompression;
}

std::vector<uint8_t> TimestampManager::binaryCompressLocOffsets(
        std::unordered_map<int, std::vector<std::pair<int, int>>> offsets) {

    int size = 0;

    //Sort unordered map
    std::map<int, std::vector<std::pair<int, int>>> ordered(offsets.begin(), offsets.end());

    std::vector<unsigned char> allColumnsCompressed;

    // Loop through all columns
    int globID = 0;


    // TODO: Count shouldn't be hardcoded here
    std::vector<int> bestSchemes(ordered.size(), -1);

    for (const auto &list: ordered) {

        std::vector<unsigned char> bestCompression;

        // Loop through all compression schemes and pick the one that
        // gives the best compression
        for (int schemeID = 0; schemeID < compressionSchemes.size(); schemeID++) {
            BitVecBuilder builder;
            builder.currentByte = 0;
            builder.remainingBits = 8;
            builder.bytesCounter = 0;
            const auto scheme = compressionSchemes.at(schemeID);

            // Compress scheme ID.
            // Always spend 8 bits on this value, as, when decompressing, we
            // don't have a compression scheme for the first value.
            // This allows for up to 256 different schemes.
            appendBits(&builder, schemeID, 8);

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

            std::cout << "GlobID: " << globID << " scheme: " << schemeID << " size: "
                      << builder.bytes.size() << std::endl;

            // Stop trying if size becomes larger than what we have already stored
//            if (builder.bytes.size() >= bestCompression.size() && !bestCompression.empty()) {
//                break;
//            }

            // Update the chosen compression if the current scheme is better
            size = builder.bytes.size();
            if (size < bestCompression.size() || bestCompression.empty()) {
                bestSchemes.at(globID) = schemeID;
                bestCompression = builder.bytes;
            }
        }

        globID++;

    }

    // Run through everything again, use the best compression scheme found above and append to the same builder
    BitVecBuilder finalCompression;
    finalCompression.currentByte = 0;
    finalCompression.remainingBits = 8;
    finalCompression.bytesCounter = 0;
    for (int i = 0; i < ordered.size(); i++) {
        auto scheme = compressionSchemes.at(bestSchemes.at(i));
        appendBits(&finalCompression, bestSchemes.at(i), 8);
        scheme(&finalCompression, latestTimestamps.at(i).timestampFirst);
        std::cout << "i: " << i << " " << "FIRST TIMESTAMP: "
                  << latestTimestamps.at(i).timestampFirst << std::endl;
        for (auto j: ordered[i + 1]) {
            scheme(&finalCompression, j.first);
            scheme(&finalCompression, j.second);
        }
        appendAZeroBit(&finalCompression);
    }
    finalCompression.bytes.push_back(finalCompression.currentByte);

    grid(finalCompression.bytes);
    return allColumnsCompressed;
}


std::vector<int>
TimestampManager::grid(std::vector<uint8_t> values) {
    int globID = 0;
    std::map<int, std::vector<std::tuple<int, int>>> offsetsDecompressed;
    std::vector<int> temp;
    bool firstTimestamp = true;
    std::vector<int> decompressed;
    BitReader bitReader = tryNewBitReader(values, values.size());
    int leadingZeros = 255;
    int trailingZeros = 0;
    uint8_t schemeID = readBits(&bitReader, 8);
    decompressed.push_back(schemeID);

    for (int i = 0; i < values.size()+542; i++) {  // <==== values.size()+300 skal rettes til antallet af dekomprimerede værdier
        int currentValue = 0;
        if (readBit(&bitReader)) {
            if (readBit(&bitReader)) {
                if (readBit(&bitReader)) {
                    if (readBit(&bitReader)) {
                        if (readBit(&bitReader)) {
                            if (readBit(&bitReader)) {
                                if (readBit(&bitReader)) {
                                    //1111111
                                    currentValue = readBits(&bitReader, 32);
                                    goto done;
                                }
                                //1111110
                                currentValue = readBits(&bitReader, 13);
                                goto done;
                            }
                            //111110
                            currentValue = readBits(&bitReader, 11);
                            goto done;
                        }
                        //11110
                        currentValue = readBits(&bitReader, 9);
                        goto done;
                    }
                    //1110
                    currentValue = readBits(&bitReader, 7);
                    goto done;
                }
                //110
                currentValue = readBits(&bitReader, 5);
                goto done;
            }
            //10
            currentValue = readBits(&bitReader, 3);
            goto done;
        } else {
            uint8_t schemeID = readBits(&bitReader, 8);
            firstTimestamp = true;
            decompressed.push_back(schemeID);
            globID++;
            continue;
        }
        done:
        if (!firstTimestamp) {
            temp.push_back(currentValue);
        }
        firstTimestamp = false;
        if (temp.size() == 2) {
            offsetsDecompressed[globID].push_back(std::make_pair(temp.at(0), temp.at(1)));
            temp.clear();
        };
    }

    return decompressed;
}
