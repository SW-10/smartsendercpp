#include <sstream>
#include "ReaderManager.h"
#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <functional>

ReaderManager::ReaderManager(std::string configFile)
        : configManager(configFile), modelManager(*configManager.getTimeSeriesColumns()) {
    this->csvFileStream.open(this->configManager.getInputFile()/*"../Cobham_hour.csv"*/, std::ios::in);

    // Initialise counters
    textCount = 0;
    valuesCount = 0;
    timestampCount = 0;
    positionCount = 0;

    // Initialise all elements in the map
    for(int i = 0; i < configManager.getNumberOfCols(); i ++){
        std::get<0>(myMap[i]) = [&](void* in) { return std::make_pair(CompressionType::NONE, 0);};
    }

    // TODO: Change all test-functions
    // Handle time series columns
    for(auto &c : *configManager.getTimeSeriesColumns()){
        std::get<0>(myMap[c.col]) = [&](void* in) {
            test("time series column ");
            return std::pair(CompressionType::VALUES, ++valuesCount);
        };
    }

    // Handle text series columns
    for(const auto &c : configManager.getTextColumns()){
        std::get<0>(myMap[c]) = [&](void* in) {
            test("text series column ");
            return std::make_pair(CompressionType::TEXT, ++textCount);
        };
    }

    // Handle time stamp columns
    auto timestampCol = configManager.getTimestampColumn();
    std::get<0>(myMap[timestampCol]) =  [&](void* in) {
        auto val = std::stoi(*static_cast<std::string*>(in));
        timestampManager.compressTimestamps( val, timestampCount );
        return std::make_pair(CompressionType::TIMESTAMP, ++timestampCount);
    };

    //Handle position columns
    if(configManager.getContainsPosition()){
        auto latCol  = configManager.getLatColumn();
        auto longCol = configManager.getLongColumn();

        //Pak lat og long sammen i et pair i stedet for at kalde dem separat
        std::get<0>(myMap[latCol->col]) = [&](void* in) {
            test("lat column ");
            return std::pair(CompressionType::POSITION, ++positionCount);
        };
        std::get<0>(myMap[longCol->col]) = [&](void* in) {
            test("long column ");
            return std::pair(CompressionType::POSITION, ++positionCount);
        };
    }
    
}

void ReaderManager::runCompressor() {
    std::vector<std::string> row;
    std::string line, word;

    std::getline(this->csvFileStream, line);

    while(!this->csvFileStream.eof()){
        row.clear();
        std::getline(this->csvFileStream, line);
        std::stringstream s(line);

        int count = 1;
        while (std::getline(s, word, ',')){
            auto mapElement = myMap.find(count); //Get element in map

            //Get the lambda function from the map.
            // 0th index in second() contains the  lambda function responsible for calling further compression methods
            auto compressFunction = std::get<0>(mapElement->second);

            CompressionType ct = compressFunction(&word).first; // Call the compression function

            std::get<1>(mapElement->second) = ct; // Update the compression type in the map


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


    std::cout << a << ", " << b << std::endl;

    std::cout << timestampManager.getTimestampFromIndex(b) << std::endl;
}