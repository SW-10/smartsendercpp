#include <sstream>
#include "ReaderManager.h"
#include <vector>
#include <string>
#include <iostream>
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
        std::get<0>(myMap[c.col-1]) = [this, i, &c](std::string* in, int &lineNum) {
            timestampManager.makeLocalOffsetList(lineNum, c.col-1); //c.col is the global ID
            test("time series column ");
            return CompressionType::VALUES;
        };
        std::get<1>(myMap[c.col-1]) = CompressionType::VALUES;
        std::get<2>(myMap[c.col-1]) = i;         // Store 'local' ID
        i++;
        std::cout << "C.COL: " << c.col-1 << std::endl;
    }

    // Handle text series columns
    i = 0;
    for(const auto &c : *configManager.getTextColumns()){
        std::get<0>(myMap[c-1]) = [this, i](std::string* in, int &lineNum) {
            test("text series column ");
            return CompressionType::TEXT;
        };

        std::get<2>(myMap[c-1]) = i;        // Store 'local' ID
        i++;
    }

    // Handle time stamp columns
    i = 0;
    auto timestampCol = configManager.getTimestampColumn() - 1;
    std::get<0>(myMap[timestampCol]) =  [this, i](std::string* in, int &lineNum) {
        timestampManager.compressTimestamps( std::stoi(*in) );
        return CompressionType::TIMESTAMP;
    };

    //Handle position columns
    if(configManager.getContainsPosition()){
        auto latCol  = configManager.getLatColumn()-1;
        auto longCol = configManager.getLongColumn()-1;

        //Pak lat og long sammen i et pair i stedet for at kalde dem separat
        std::get<0>(myMap[latCol->col-1]) = [&](std::string* in, int &lineNum) {
            if(bothLatLongSeen){ // Ensure that both lat and long are available before calling the function
                test("lat column ");
                bothLatLongSeen = false;
            } else {
                bothLatLongSeen = true;
            }
            return CompressionType::POSITION;
        };
        std::get<0>(myMap[longCol->col-1]) = [&](std::string* in, int &lineNum) {
            if(bothLatLongSeen){ // Ensure that both lat and long are available before calling the function
                test("long column ");
                bothLatLongSeen = false;
            } else {
                bothLatLongSeen = true;
            }
            return CompressionType::POSITION;
        };
    }
    
}

void ReaderManager::runCompressor() {
    std::vector<std::string> row;
    std::string line, word;

    std::getline(this->csvFileStream, line);

    int lineNumber = 0;
    while(!this->csvFileStream.eof()){
        lineNumber++;
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

//            std::cout << "COUNT: " << count << std::endl;
            count++;
        }      
      }
    this->csvFileStream.close();


//     The following lines should probably be called elsewhere, but I'll leave it here for now ...
//     TODO: Figure out where this stuff goes

//        for(auto c : timestampManager.getOffsetList()){
//            std::cout << c.first << ":" << c.second << std::endl;
//        }
//    std::cout << timestampManager.getFirstTimestamp() << std::endl;
//    timestampManager.reconstructTimestamps();
//
    int a, b;
    auto res = timestampManager.calcIndexRangeFromTimestamps(1645153465,1645311865, a,b);


    //std::cout << a << ", " << b << std::endl;

    //std::cout << timestampManager.getTimestampFromIndex(b) << std::endl;
}