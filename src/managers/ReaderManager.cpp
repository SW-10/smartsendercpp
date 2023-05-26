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
#include <cstdlib>

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

                #ifndef NDEBUG
                timeseries[c.col].push_back(std::make_pair(timestampManager.timestampCurrent->data, value));
                #endif
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
    std::cerr <<
    budgetManager.modelSizeTotal  << "," <<
    budgetManager.huffmanSizeTotal   << "," <<
    budgetManager.weightedSum / budgetManager.totalLength << "," <<
    0.2;




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

    decompressModels();

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
        BudgetManager::writeModelsToCsv(selected);
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

struct Model{
    int start;
    int end;
    float errorBound;
    int length;
    int MID;
    int CID;
    std::vector<uint8_t> values;
};

void ReaderManager::decompressModels(){
    std::fstream csvFileStream;
    csvFileStream.open("../models.csv", std::ios::in);
    if(!csvFileStream)
    {
        std::cout << "Can't open file!" << std::endl;
    } else {
        std::cout << "File opened successfully" << std::endl;
    }

    std::vector<Model> models;

    std::vector<std::string> row;
    std::string line, word;
//    std::getline(csvFileStream, line);
    int lineNO = 0;

    while (std::getline(csvFileStream, line)) {
        row.clear();
        std::stringstream s(line);
        Model m;
        int count = 0;
        while (std::getline(s, word, ',')) {
            switch(count){
                case 0:
                    m.start = std::stoi(word);
                    break;
                case 1:
                    m.end = std::stoi(word);
                    break;
                case 2:
                    m.errorBound = std::stof(word);
                    break;
                case 3:
                    m.length = std::stoi(word);
                    break;
                case 4:
                    m.MID = std::stoi(word);
                    break;
                case 5:
                    m.CID = std::stoi(word);
                    break;
                case 6:
                    std::stringstream ss(word);
                    std::string token;
                    while (std::getline(ss, token, '-')) {
                        m.values.push_back(std::stoi(token));
                    }
                    break;
            }

            count++;
            if(count > 6){
                count = 0;
            }
        }
        lineNO++;
        models.push_back(m);
    }
    double error = 0;
    int totalValues = 0;
    Swing swing(error);
    int countModel = 0;
    std::map<int, int> trys;
    for (auto col: configManager.timeseriesCols){
        trys[col.col] = 0;
    }

    for(auto const& m : models){
        trys[m.CID] += m.length;
    }

    for (auto now: trys){
        //std::cout << now.second << std::endl;
        if (now.second > 3674){
            std::cout << now.first << std::endl;
        }


    }

    for(auto const& m : models){
        //std::cout << "countModel: " << countModel << std::endl;
        auto &tsInstance = timeseries[m.CID];
        if (m.CID == 22 || m.CID == 23){
            continue;
        }
        switch(m.MID){
            case 0: {
                //pmc
                //std::cout << "pmc!!!" << std::endl;
                float val = bytesToFloat(m.values);

                std::vector<int> timestamps;
                std::vector<int> originalValues;
                for(const auto elem : tsInstance){
                    timestamps.push_back(elem.first);
                    originalValues.push_back(elem.second);
                }

                // ORIGINAL TIMESTAMPS
                auto first = timestamps.begin();
                auto last  = timestamps.begin()+m.length;
                std::vector<long> timestampsSub(first, last);

                // ORIGINAL VALEUS
                auto firstV = originalValues.begin();
                auto lastV  = originalValues.begin()+m.length;
                std::vector<float> originalValuesSub(firstV, lastV);

                auto res = PmcMean::gridPmcMean(val, m.length);
                calcActualError(originalValuesSub, res, 0, m.errorBound, m.CID);

                break;
            }
            case 1: {
                //swing
                // 4 første bytes af values og 4 næste bytes i values
                //std::cout << "hej" << std::endl;
                std::vector<int> timestamps;
                std::vector<int> originalValues;
                for(const auto elem : tsInstance){
                    timestamps.push_back(elem.first);
                    originalValues.push_back(elem.second);
                }

                auto floats = bytesToFloats(m.values);

                bool up = m.values.at(8);

                // ORIGINAL TIMESTAMPS
                auto first = timestamps.begin();
                auto last  = timestamps.begin()+m.length;
                std::vector<long> timestampsSub(first, last);

                // ORIGINAL VALEUS
                auto firstV = originalValues.begin();
                auto lastV  = originalValues.begin()+m.length;
                std::vector<float> originalValuesSub(firstV, lastV);

                auto res = swing.gridSwing(floats.at(0), floats.at(1),up,timestampsSub, m.length);
//                std::cout << "float 0: " << floats.at(0) << ", float 1: " << floats.at(1) << " error: " << m.errorBound << std::endl;
//                for(int i = 0; i < res.size(); i++){
//                    std::cout << "Swing res: " << res.at(i) << " ori: " << originalValuesSub.at(i) << std::endl;
//                }
                calcActualError(originalValuesSub, res, 1, m.errorBound, m.CID);
                break;
            }
            case 2: {
                //gorilla
                std::vector<int> timestamps;
                std::vector<int> originalValues;
                for(const auto elem : tsInstance){
                    timestamps.push_back(elem.first);
                    originalValues.push_back(elem.second);
                }

                // ORIGINAL TIMESTAMPS
                auto first = timestamps.begin();
                auto last  = timestamps.begin()+m.length;
                std::vector<long> timestampsSub(first, last);

                // ORIGINAL VALEUS
                auto firstV = originalValues.begin();
                auto lastV  = originalValues.begin()+m.length;
                std::vector<float> originalValuesSub(firstV, lastV);


                /*auto res = Gorilla::gridGorilla(m.values, m.values.size(), m.length);

                for(int i = 0; i < res.size(); i++){
                    if(res.at(i) != originalValuesSub.at(i)){
//                        std::cout << "!!!!!!!!!!!!!!!! res: " << res.at(i) << " ori: " << originalValuesSub.at(i) << std::endl;
                    } else {
//                        std::cout << ":-)" << std::endl;
                    }
                }*/
//                std::cout<<std::endl;
                break;
            }

        }
        if (tsInstance.size() < m.length){
            std::cout << "nonooonononono" << std::endl;
        }
        tsInstance.erase(tsInstance.begin(), tsInstance.begin()+m.length);
        int f = 0;
        countModel++;
    }

}

float ReaderManager::bytesToFloat(std::vector<uint8_t> bytes) {
    union ByteFloatUnion{
        uint8_t bytes[4];
        float floatValue;
    };

    ByteFloatUnion u;
    std::copy(bytes.begin(), bytes.end(), u.bytes);
    return u.floatValue;
}

std::vector<float> ReaderManager::bytesToFloats(std::vector<uint8_t> bytes) {

    union ByteFloatUnion{
        uint8_t bytes[8];
        float floatValue[2];
    };

    ByteFloatUnion u;
    std::copy(bytes.begin(), bytes.end()-1, u.bytes); // -1 because the last element is up/down indicator

    std::vector<float> result;
    result.push_back(u.floatValue[0]);
    result.push_back(u.floatValue[1]);
    return result;
}

#ifndef NDEBUG
float ReaderManager::calcActualError(const std::vector<float> &original, const std::vector<float> &reconstructed,
                                     int modelType, float errorbound, int col) {
    int size = original.size();
    float result = 0;
    for(int i = 0; i < size; i++){
        float error = std::abs( (reconstructed.at(i) / original.at(i)  * 100 ) - 100);
        if(error > errorbound){
            if(std::abs(error-errorbound) < 0.01){
                //std::cout << "nonon" << std::endl;
            }
            else {
                std::cout << modelType << " ERROR: " << error  << ", ERROR BOUND: " << errorbound << " Col: " << col << std::endl;
            }

        }

        //CHECK(std::fabs(p.error - 7.5) < 0.01);
    }
    //std::cout << std::endl;
    return result;
}
#endif