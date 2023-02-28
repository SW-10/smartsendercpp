#include "../utils/Utils.h"
#include "TimestampManager.h"
#include <iostream>
#include <algorithm>

TimestampManager::TimestampManager(){

}

void TimestampManager::compressTimestamps(int timestamp){
//    timestampCount++;
//    timestamps.push_back(timestamp);
    timestampCurrent = timestamp;
    if(!readyForOffset) firstTimestamp = timestamp;
    if(readyForOffset){

        // Offset = Difference between current and previous timestamp
        currentOffset = timestampCurrent - timestampPrevious ;

        // Insert new offset if first element or if current offset is not equal to previous offset
        // Else, increase counter for current offset
        if(offsetList.empty() || currentOffset != offsetList[offsetList.size()-1].first){
            offsetList.emplace_back(currentOffset, 1);
        } else {
            offsetList[offsetList.size()-1].second++;
        }
    }

    timestampPrevious = timestampCurrent;
    readyForOffset = true;
}

std::vector<int> TimestampManager::reconstructTimestamps() {
    std::vector<int> reconstructed;
    reconstructed.push_back(firstTimestamp);
    int current = firstTimestamp;
    for(auto o : offsetList){
        for(int i = 0; i < o.second; i++){
            current += o.first;
            reconstructed.push_back(current);
        }
    }

    return reconstructed;
}

bool TimestampManager::calcIndexRangeFromTimestamps(int first, int second, int& first_out, int& second_out){
    int out1, out2;
    bool success = true;
    std::vector<int> reconstructed = reconstructTimestamps();
    out1 = Utils::BinarySearch(reconstructed, first);
    out2 = Utils::BinarySearch(reconstructed, second);

    if(out1 == -1){
        std::cout << "First value (" << first << ")  in range not found";
        success = false;
    }
    if(out2 == -1){
        std::cout << "Second value (" << second << ")  in range not found";
        success = false;
    }
    if(first >= second){
        success = false;
    }

    first_out = out1;
    second_out = out2;

    return success;
}

int TimestampManager::getTimestampFromIndex(int index) {
    int count = 0;
    int current = firstTimestamp;
    for(auto o : offsetList){
        for(int i = 0; i < o.second; i++){
            if(count == index){
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
