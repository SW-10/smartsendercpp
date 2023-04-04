#include <sstream>
#include "ReaderManager.h"
#include <vector>
#include <string>
#include <iostream>
#include "../utils/Timer.h"
#include <functional>

//int Observer::static_number_ = 0;
//Timekeeper *timekeeper = new Timekeeper;

ReaderManager::ReaderManager(std::string configFile, Timekeeper &timekeeper)
        : configManager(configFile), timestampManager(configManager, timekeeper),
          modelManager(configManager.timeseriesCols, configManager.textCols,
                       timestampManager) {
    timekeeper.Attach(this);
    this->csvFileStream.open(
            "../" + this->configManager.inputFile/*"../Cobham_hour.csv"*/,
            std::ios::in);

    bothLatLongSeen = false;

    // Initialise all elements in the map
    for (int i = 0; i < configManager.totalNumberOfCols; i++) {
        std::get<0>(myMap[i]) = [](std::string *in,
                                   int &lineNum) { return CompressionType::NONE; };
        std::get<1>(myMap[i]) = CompressionType::NONE;

    }

    // TODO: Change all test-functions
    // Handle time series columns
    int i = 0;
    for (const auto &c: configManager.timeseriesCols) {
        std::get<0>(myMap[c.col]) = [this, i, &c](std::string *in,
                                                  int &lineNum) {
            if (!in->empty()) {
                timestampManager.makeLocalOffsetList(lineNum,
                                                     c.col); //c.col is the global ID
                modelManager.fitSegment(i, std::stof(*in),
                                        timestampManager.timestampCurrent);
            }
            return CompressionType::VALUES;
        };
        std::get<1>(myMap[c.col]) = CompressionType::VALUES;
        std::get<2>(myMap[c.col]) = i;         // Store 'local' ID
        i++;
    }

    // Handle text series columns
    i = 0;
    for (const auto &c: configManager.textCols) {
        std::get<0>(myMap[c]) = [this, i](std::string *in, int &lineNum) {
            if (!in->empty()) {
                modelManager.fitTextModels(i, *in);
            }
            return CompressionType::TEXT;
        };

        std::get<2>(myMap[c]) = i;        // Store 'local' ID
        i++;
    }

    // Handle time stamp columns
    auto timestampCol = configManager.timestampCol;
    std::get<0>(myMap[timestampCol]) = [this](std::string *in, int &lineNum) {
        timestampManager.compressTimestamps(std::stoi(*in));
        return CompressionType::TIMESTAMP;
    };

    // Handle position columns
    if (configManager.containsPosition) {
        auto latCol = &configManager.latCol;
        auto longCol = &configManager.longCol;

        // Package lat and long into a pair instead of calling them separately.
        std::get<0>(myMap[latCol->col]) = [this, latCol](
                std::string *in, int &lineNum) {
            if (bothLatLongSeen) { // Ensure that both lat and long are available before calling the function
                bothLatLongSeen = false;
            } else {
                bothLatLongSeen = true;
            }
            if (!in->empty()) {
                timestampManager.makeLocalOffsetList(lineNum,
                                                     latCol->col); //c.col is the global ID
            }
            return CompressionType::POSITION;
        };
        std::get<0>(myMap[longCol->col]) = [this, longCol](
                std::string *in, int &lineNum) {
            if (bothLatLongSeen) { // Ensure that both lat and long are available before calling the function
                bothLatLongSeen = false;
            } else {
                bothLatLongSeen = true;
            }
            if (!in->empty()) {
                timestampManager.makeLocalOffsetList(lineNum,
                                                     longCol->col); //c.col is the global ID
            }
            return CompressionType::POSITION;
        };
    }
}

// Overwritten function in the observer pattern
// Used to receive a notification from the timekeeper about a new interval
void ReaderManager::Update(const std::string &message_from_subject) {
    if(message_from_subject == "New interval"){
        newInterval = true;
    }
}

void ReaderManager::runCompressor() {
    std::vector<std::string> row;
    std::string line, word;

    std::getline(this->csvFileStream, line);
    Timer time;
    time.begin();

    int lineNumber = 0;
    int timestampFlusherPenalty = 50;
    int lastTimestampFlush = 0;
    int hej = 0;
    while (!this->csvFileStream.eof()) {
        row.clear();
        std::getline(this->csvFileStream, line);
        std::stringstream s(line);

        int count = 0;

        while (std::getline(s, word, ',')) {

            auto mapElement = myMap.find(count); //Get element in map

            // Get the lambda function from the map.
            // 0th index in second() contains the  lambda function responsible for calling further compression methods
            auto compressFunction = std::get<0>(mapElement->second);

            // Call the compression function
            CompressionType ct = compressFunction(&word,lineNumber);

            // Run code that handles new intervals directly after reading the timestamp
            // newInterval is set to true when timekeeper sends a message which is received by the
            // Update() function in ReaderManager.cpp
            if(newInterval){
                std::cout << "Hello from reader " << hej << std::endl;
                newInterval = false;
                hej++;
            }

            // Update the compression type in the map
            std::get<1>(mapElement->second) = ct;
            count++;
            // TODO: Adjust penalty dynamically
            if ((lastTimestampFlush + timestampFlusherPenalty) == lineNumber){
                lastTimestampFlush = lineNumber;
                bool didFlush = modelManager.calculateFlushTimestamp();
                if (didFlush){
                    timestampFlusherPenalty -= 5;
                }
                else {
                    timestampFlusherPenalty +=5;
                }
            }
        }
        lineNumber++;
    }
    this->csvFileStream.close();
    std::cout << "Size of local offset list: " << sizeof(timestampManager.localOffsetList) << std::endl;
    std::cout << "Time Taken: " << time.end() << " ms" << std::endl;
//    std::cout << "size glob: " << timestampManager.binaryCompressGlobOffsets(timestampManager.offsetList).size() << std::endl;
//    std::cout << "size loc : " << timestampManager.binaryCompressLocOffsets(timestampManager.localOffsetList).size() << std::endl;
    std::cout << "size glob: " << timestampManager.getSizeOfGlobalOffsetList() << std::endl;
    std::cout << "size loc : " << timestampManager.getSizeOfLocalOffsetList() << std::endl;
    std::cout << "size of int: " << sizeof(int) << std::endl;


    std::cout << "size loc : " << timestampManager.binaryCompressLocOffsets2(timestampManager.localOffsetList).size() << std::endl;

}

