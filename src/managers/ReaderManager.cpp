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
          modelManager(configManager.timeseriesCols,
                       timestampManager),
                       budgetManager(modelManager, configManager, timestampManager, configManager.budget, configManager.maxAge),
          outlierDetector(configManager) {
    timekeeper.Attach(this);
    timekeeper.intervalSeconds = &configManager.chunkSize;
    this->csvFileStream.open(
            "../" + this->configManager.inputFile/*"../Cobham_hour.csv"*/,
            std::ios::in);

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

            #ifndef NDEBUG
            datasetTotalSize += sizeof(float) + sizeof(int); // float = value, int = timestamp
            #endif

            if (!in->empty()) {
                if(this->budgetManager.outlierCooldown[i] > 0){
                    this->budgetManager.outlierCooldown[i]--;
                }

                float value = std::stof(*in);
                if(outlierDetector.addValueAndDetectOutlier(i, value)){
                  //std::cout << "Outlier detected on line " << lineNum + 2 << " in column " << char(64 + i + 2) << std::endl;
                    this->budgetManager.decreaseErrorBounds(i);
                  this->budgetManager.outlierCooldown[i] = this->budgetManager.cooldown;
                }
                timestampManager.makeLocalOffsetList(lineNum,
                                                     c.col); //c.col is the global ID

                //timestampManager.deltaDeltaCompress(lineNum, c.col);
                modelManager.fitSegment(i, value,timestampManager.timestampCurrent);

                //If adjusted model for current time series exist, fit!
                if(budgetManager.adjustableTimeSeries.find(i) != budgetManager.adjustableTimeSeries.end()){
                    budgetManager.adjustingModelManager.fitSegment(budgetManager.adjustableTimeSeries[i], value,
                                                                   timestampManager.timestampCurrent);
                }
            }
            return CompressionType::VALUES;
        };
        std::get<1>(myMap[c.col]) = CompressionType::VALUES;
        std::get<2>(myMap[c.col]) = i;         // Store 'local' ID
        i++;
    }

    // Handle time stamp columns
    auto timestampCol = configManager.timestampCol;
    std::get<0>(myMap[timestampCol]) = [this](std::string *in, int &lineNum) {
        timestampManager.compressTimestamps(std::stoi(*in));
        return CompressionType::TIMESTAMP;
    };
}

// Overwritten function in the observer pattern
// Used to receive a notification from the timekeeper about a new interval
void ReaderManager::Update(const std::string &message_from_subject) {
    if(message_from_subject == "New interval"){
        newInterval = true;
    }
}

void ReaderManager::runCompressor() {
    std::ofstream myfile;
    myfile.open ("../models.csv", std::ios_base::out);
    myfile.close();
    #ifdef linux
    ConnectionAddress address("0.0.0.0", 9999);
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
                //std::cout << "timebefore" << lineNumber << std::endl;
                budgetManager.endOfChunkCalculations();
                //std::cout << "timenow" << lineNumber << std::endl;
                newInterval = false;
            }

            // Update the compression type in the map
            std::get<1>(mapElement->second) = ct;
            count++;
            // TODO: Adjust penalty dynamically
            if ((lastTimestampFlush + timestampFlusherPenalty) == lineNumber){
                lastTimestampFlush = lineNumber;
                Node* latestUsedOriginal = modelManager.calculateFlushTimestamp();
                Node* latestUsedAdjustable = budgetManager.adjustingModelManager.calculateFlushTimestamp();
                bool didFlush = false;
                if (latestUsedOriginal != nullptr && latestUsedAdjustable != nullptr){
                    if (latestUsedOriginal->data < latestUsedAdjustable->data){
                        didFlush = timestampManager.flushTimestamps(latestUsedOriginal);
                    }
                    else {
                        didFlush = timestampManager.flushTimestamps(latestUsedAdjustable);
                    }
                }
                else if(latestUsedAdjustable == nullptr && latestUsedOriginal != nullptr){
                    didFlush = timestampManager.flushTimestamps(latestUsedOriginal);
                }
                else if (latestUsedAdjustable != nullptr){
                    didFlush = timestampManager.flushTimestamps(latestUsedOriginal);
                }

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

    finaliseCompression();

    this->csvFileStream.close();
    //timestampManager.finishDeltaDelta();
    std::cout << "Size of local offset list: " << timestampManager.getSizeOfLocalOffsetList() * sizeof(int) << " bytes" << std::endl;
    std::cout << "Size of global offset list: " << timestampManager.getSizeOfGlobalOffsetList() * sizeof(int) << " bytes" << std::endl;

    std::cout << "Time Taken: " << time.end() << " ms" << std::endl;
    std::cout << "size glob: " << timestampManager.getSizeOfGlobalOffsetList() << std::endl;
    std::cout << "size loc : " << timestampManager.getSizeOfLocalOffsetList() << std::endl;
    std::cout << "size of int: " << sizeof(int) << std::endl;




#ifndef NDEBUG
    std::cout << "HUFFMAN TOTAL SIZE: " << budgetManager.huffmanSizeTotal <<  " bytes" << std::endl;
    std::cout << "MODEL   TOTAL SIZE: " << budgetManager.modelSizeTotal   <<  " bytes" << std::endl;
    std::cout << "DATASET TOTAL SIZE: " << datasetTotalSize   <<  " bytes" << std::endl;
#endif

    //std::cout << "size loc : " << timestampManager.binaryCompressLocOffsets2(timestampManager.localOffsetList).size() << std::endl;
    //timestampManager.reconstructDeltaDelta();

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

void ReaderManager::finaliseCompression() {
    for(int i = 0; i < configManager.timeseriesCols.size(); i++){
        modelManager.forceModelFlush(i);
    }

    for(auto const  &selected : modelManager.selectedModels){
        for(auto const &s : selected){
            if(s.send){
                budgetManager.modelSizeTotal += budgetManager.sizeOfModels;
                budgetManager.modelSizeTotal += s.values.size();
            }
        }
    }


    timestampManager.flushTimestamps(timestampManager.timestampCurrent);

    if (!timestampManager.localOffsetListToSend.empty()){
        Huffman huffmanLOL;
        huffmanLOL.runHuffmanEncoding(timestampManager.localOffsetListToSend, false);
        huffmanLOL.encodeTree();

        // CALC SIZE OF HUFFMAN
        budgetManager.huffmanSizeTotal += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();
        timestampManager.localOffsetListToSend.clear();
    }
    if(!timestampManager.globalOffsetListToSend.empty()){
        Huffman huffmanGOL;
        huffmanGOL.runHuffmanEncoding(timestampManager.globalOffsetListToSend, false);
        huffmanGOL.encodeTree();

        // CALC SIZE OF HUFFMAN
        budgetManager.huffmanSizeTotal += huffmanGOL.huffmanBuilder.bytes.size() + huffmanGOL.treeBuilder.bytes.size();
        timestampManager.globalOffsetListToSend.clear();
    }
}
