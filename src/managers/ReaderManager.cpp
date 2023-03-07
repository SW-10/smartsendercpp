#include <sstream>
#include "ReaderManager.h"
#include <vector>
#include <string>
#include <iostream>
#include "../utils/Timer.h"
#include <utility>
#include <functional>

ReaderManager::ReaderManager(std::string configFile)
        : configManager(configFile), timestampManager(configManager), modelManager(*configManager.getTimeSeriesColumns(), *configManager.getTextColumns(), timestampManager) {
    this->csvFileStream.open(this->configManager.getInputFile()/*"../Cobham_hour.csv"*/, std::ios::in);

    // Initialise counters
    textCount = 0;
    valuesCount = 0;
    timestampCount = 0;
    positionCount = 0;

    bothLatLongSeen = false;

    // Initialise all elements in the map
    for(int i = 0; i < configManager.getTotalNumberOfCols(); i ++){
        std::get<0>(myMap[i]) = [this](std::string* in, int &lineNum) { return CompressionType::NONE;};
        std::get<1>(myMap[i]) = CompressionType::NONE;

    }

    // TODO: Change all test-functions
    // Handle time series columns
    int i = 0;
    for(const auto &c : *configManager.getTimeSeriesColumns()){
        std::get<0>(myMap[c.col]) = [this, i, &c](std::string* in, int &lineNum) {
            if (!in->empty()){
                timestampManager.makeLocalOffsetList(lineNum, c.col); //c.col is the global ID
                test("time series column ");
                modelManager.fitTimeSeriesModels(i, std::stof(*in), timestampManager.getCurrentTimestamp());
            }
            return CompressionType::VALUES;
        };
        std::get<1>(myMap[c.col]) = CompressionType::VALUES;
        std::get<2>(myMap[c.col]) = i;         // Store 'local' ID
        i++;
    }

    // Handle text series columns
    i = 0;
    for(const auto &c : *configManager.getTextColumns()){
        std::get<0>(myMap[c]) = [this, i](std::string* in, int &lineNum) {
            if (!in->empty()){
                modelManager.fitTextModels(i, *in);
            }
            return CompressionType::TEXT;
        };

        std::get<2>(myMap[c]) = i;        // Store 'local' ID
        i++;
    }

    // Handle time stamp columns
    i = 0;
    auto timestampCol = configManager.getTimestampColumn();
    std::get<0>(myMap[timestampCol]) =  [this, i, timestampCol](std::string* in, int &lineNum) {
        //timestampManager.makeLocalOffsetList(lineNum, timestampCol); //c.col is the global ID
        timestampManager.compressTimestamps( std::stoi(*in) );
        return CompressionType::TIMESTAMP;
    };

    //Handle position columns
    if(configManager.getContainsPosition()){
        auto latCol  = configManager.getLatColumn();
        auto longCol = configManager.getLongColumn();

        //Pak lat og long sammen i et pair i stedet for at kalde dem separat
        std::get<0>(myMap[latCol->col]) = [this, latCol](std::string* in, int &lineNum) {
            if(bothLatLongSeen){ // Ensure that both lat and long are available before calling the function
                test("lat column ");
                bothLatLongSeen = false;
            } else {
                bothLatLongSeen = true;
            }
            timestampManager.makeLocalOffsetList(lineNum, latCol->col); //c.col is the global ID
            return CompressionType::POSITION;
        };
        std::get<0>(myMap[longCol->col]) = [this, longCol](std::string* in, int &lineNum) {
            if(bothLatLongSeen){ // Ensure that both lat and long are available before calling the function
                test("long column ");
                bothLatLongSeen = false;
            } else {
                bothLatLongSeen = true;
            }
            timestampManager.makeLocalOffsetList(lineNum, longCol->col); //c.col is the global ID
            return CompressionType::POSITION;
        };
    }
    
}

void ReaderManager::runCompressor() {
    std::vector<std::string> row;
    std::string line, word;

    std::getline(this->csvFileStream, line);
    Timer time;
    time.begin();

    int lineNumber = 0;
    while(!this->csvFileStream.eof()){
        row.clear();
        std::getline(this->csvFileStream, line);
        std::stringstream s(line);

        int count = 0;
        while (std::getline(s, word, ',')){
            auto mapElement = myMap.find(count); //Get element in map

            //Get the lambda function from the map.
            // 0th index in second() contains the  lambda function responsible for calling further compression methods
            auto compressFunction = std::get<0>(mapElement->second);
            
            CompressionType ct = compressFunction(&word, lineNumber); // Call the compression function

            std::get<1>(mapElement->second) = ct; // Update the compression type in the map


            count++;
            //std::cout << "c: " << count << std::endl;
        }
        //std::cout << "line: " << lineNumber << std::endl;
        lineNumber++;
      }
    this->csvFileStream.close();
    std::cout << "Time Taken: " << time.end() << std::endl;


//     The following lines should probably be called elsewhere, but I'll leave it here for now ...
//     TODO: Figure out where this stuff goes

//        for(auto c : timestampManager.getOffsetList()){
//            std::cout << c.first << ":" << c.second << std::endl;
//        }
//    std::cout << timestampManager.getFirstTimestamp() << std::endl;
//    timestampManager.reconstructTimestamps();
//
    //int a, b;
    //auto res = timestampManager.calcIndexRangeFromTimestamps(1645153465,1645311865, a,b);
    //auto hej = timestampManager.getTimestampRangeForColumns(2, 0, 10);

    //auto hej2 = timestampManager.getTimestampRangeForColumnsByTimestamp(2, 1645157065, 1645203865);

    //std::cout << a << ", " << b << std::endl;

    //std::cout << timestampManager.getTimestampFromIndex(b) << std::endl;
}