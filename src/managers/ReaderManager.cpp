#include <sstream>
#include "ReaderManager.h"
#include <vector>
#include <string>
#include <iostream>
#include "../utils/Timer.h"
#include "../utils/Utils.h"
#include <functional>

//int Observer::static_number_ = 0;
//Timekeeper *timekeeper = new Timekeeper;

ReaderManager::ReaderManager(std::string configFile, Timekeeper &timekeeper)
        : configManager(configFile), timestampManager(configManager, timekeeper),
          modelManager(configManager.timeseriesCols, configManager.textCols,
                       timestampManager),
                       budgetManager(modelManager, configManager, timestampManager, configManager.budget, configManager.maxAge, &timekeeper.firstTimestamp) {
    timekeeper.Attach(this);
    timekeeper.intervalSeconds = &configManager.chunkSize;
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

                timestampManager.deltaDeltaCompress(lineNum, c.col);
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
                timestampManager.deltaDeltaCompress(lineNum, latCol->col);

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
                timestampManager.deltaDeltaCompress(lineNum, longCol->col);
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
    #ifdef linux
    //ConnectionAddress address("0.0.0.0", 9999);
    #endif

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
//            if(newInterval){
////                budgetManager.endOfChunkCalculations();
//                std::cout << "Hello from reader " << hej << std::endl;
//                newInterval = false;
//                hej++;
//            }

            // Update the compression type in the map
            std::get<1>(mapElement->second) = ct;
            count++;
            // TODO: Adjust penalty dynamically
//            if ((lastTimestampFlush + timestampFlusherPenalty) == lineNumber){
//                lastTimestampFlush = lineNumber;
//                bool didFlush = modelManager.calculateFlushTimestamp();
//                if (didFlush){
//                    timestampFlusherPenalty -= 5;
//                }
//                else {
//                    timestampFlusherPenalty +=5;
//                }
//            }
        }
//        std::cout << "Line number " << lineNumber << std::endl;

        lineNumber++;
    }
    this->csvFileStream.close();
    timestampManager.finishDeltaDelta();
    std::cout << "Size of local offset list: " << timestampManager.getSizeOfLocalOffsetList() * sizeof(int) << " bytes" << std::endl;
    std::cout << "Size of global offset list: " << timestampManager.getSizeOfGlobalOffsetList() * sizeof(int) << " bytes" << std::endl;

    std::cout << "Time Taken: " << time.end() << " ms" << std::endl;
    std::cout << "size glob: " << timestampManager.binaryCompressGlobOffsets(timestampManager.offsetList).size() * sizeof(int8_t) << std::endl;
    std::cout << "size loc : " << timestampManager.binaryCompressLocOffsets(timestampManager.localOffsetList).size() * sizeof(int8_t) << std::endl;

    timestampManager.binaryCompressLocOffsets2(timestampManager.localOffsetList);

    //std::cout << "size loc : " << timestampManager.binaryCompressLocOffsets2(timestampManager.localOffsetList).size() << std::endl;
    timestampManager.reconstructDeltaDelta();


    #ifdef linux
    /*auto table = VectorToColumnarTable(
            this->modelManager.selectedModels).ValueOrDie();

    auto recordBatch = MakeRecordBatch(table).ValueOrDie();

    ARROW_ASSIGN_OR_RAISE(auto flightClient, createClient(address))
    auto doPutResult = flightClient->DoPut(arrow::flight::FlightCallOptions(),
                        arrow::flight::FlightDescriptor{arrow::flight
                                                        ::FlightDescriptor::Path(std::vector<std::string>{"table.parquet"})}, recordBatch->schema()).ValueOrDie();

    ARROW_RETURN_NOT_OK(doPutResult.writer->WriteRecordBatch(*recordBatch));
    arrow::Status st = arrow::Status::OK();
    if (!st.ok()) {
        std::cerr << st << std::endl;
        exit(1);
    }*/

    #endif
}