#include <sstream>
#include "ReaderManager.h"
#include <vector>
#include <string>
#include <iostream>
#include "../utils/Timer.h"
#include "../utils/Utils.h"
#include "../utils/Huffman.h"
#include "../utils/OutlierDetector.h"
#include <functional>
#include <forward_list>

//int Observer::static_number_ = 0;
//Timekeeper *timekeeper = new Timekeeper;

ReaderManager::ReaderManager(std::string configFile, Timekeeper &timekeeper)
        : configManager(configFile), timestampManager(configManager, timekeeper),
          modelManager(configManager.timeseriesCols, configManager.textCols,
                       timestampManager),
                       budgetManager(modelManager, configManager, timestampManager, configManager.budget, configManager.maxAge, &timekeeper.firstTimestamp),
          outlierDetector(4.0, configManager.timeseriesCols.size()) {
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
        outlierDetector.count.push_back(0);
        outlierDetector.mean.push_back(0.0);
        outlierDetector.m2.push_back(0.0);
        std::get<0>(myMap[c.col]) = [this, i, &c](std::string *in,
                                                  int &lineNum) {
            if (!in->empty()) {
                float value = std::stof(*in);
                if(outlierDetector.addValueAndDetectOutlier(i, value)){
                  //std::cout << "Outlier detected on line " << lineNum + 2 << " in column " << char(64 + i + 2) << std::endl;
                }
                timestampManager.makeLocalOffsetList(lineNum,
                                                     c.col); //c.col is the global ID

                //timestampManager.deltaDeltaCompress(lineNum, c.col);
                modelManager.fitSegment(i, value,
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
                //timestampManager.deltaDeltaCompress(lineNum, latCol->col);

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
                //timestampManager.deltaDeltaCompress(lineNum, longCol->col);
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
                budgetManager.endOfChunkCalculations();
                newInterval = false;
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
    //timestampManager.finishDeltaDelta();
    std::cout << "Size of local offset list: " << timestampManager.getSizeOfLocalOffsetList() * sizeof(int) << " bytes" << std::endl;
    std::cout << "Size of global offset list: " << timestampManager.getSizeOfGlobalOffsetList() * sizeof(int) << " bytes" << std::endl;

    std::cout << "Time Taken: " << time.end() << " ms" << std::endl;
    std::cout << "size glob: " << timestampManager.getSizeOfGlobalOffsetList() << std::endl;
    std::cout << "size loc : " << timestampManager.getSizeOfLocalOffsetList() << std::endl;
    std::cout << "size of int: " << sizeof(int) << std::endl;


    //std::cout << "size loc : " << timestampManager.binaryCompressLocOffsets2(timestampManager.localOffsetList).size() << std::endl;
    //timestampManager.reconstructDeltaDelta();


    // Huffman-encode Local offset list
    /*Huffman huffmanLOL;
    auto vecLOL = timestampManager.flattenLOL();
//    std::vector<int> paperExample = {0,1,5,2,1,1,4,-2,
//                                     0,1,5,2,1,1,1,2,1,1,1,-2,
//                                     0,3,1,4,1,3,1,-2,
//                                     0,2,2,3,1,2,1,-2,
//                                     1,5,1,4,1,-3};
    std::vector<int> paperExample = {60, 5, 5, 1, 55, 1, 60, 4,-3};
    huffmanLOL.runHuffmanEncoding(paperExample, true);
    huffmanLOL.encodeTree();
    MinHeapNode* LOLtreeRoot  = huffmanLOL.decodeTree();
    auto LOL = huffmanLOL.decodeLOL(LOLtreeRoot, huffmanLOL.huffmanBuilder.bytes);
    std::cout << "HUFFMAN SIZE (LOL): " << huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size() << std::endl;

    // Huffman-encode Global offset list
    Huffman huffmanGOL;
    auto vecGOL = timestampManager.flattenGOL();
    huffmanGOL.runHuffmanEncoding(vecGOL, true);
    huffmanGOL.encodeTree();
    MinHeapNode* GOLtreeRoot  = huffmanGOL.decodeTree();
    auto GOL = huffmanGOL.decodeGOL(GOLtreeRoot, huffmanGOL.huffmanBuilder.bytes);
    std::cout << "HUFFMAN SIZE (GOL): " << huffmanGOL.huffmanBuilder.bytes.size() + huffmanGOL.treeBuilder.bytes.size() << std::endl;
*/
    #ifdef linux
    auto table = VectorToColumnarTable(
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
    }
    #endif
}