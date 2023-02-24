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

    // Initialise all elements in the map
    for(int i = 0; i < configManager.getNumberOfCols(); i ++){
        myMap[i] = [this](const std::string& in) {};
    }

    // TODO: Change all test-functions
    // Handle time series columns
    for(auto &c : *configManager.getTimeSeriesColumns()){
        myMap[c.col] = [this](const std::string& in) {
            test("time series column ");
        };
    }

    // Handle text series columns
    for(const auto &c : configManager.getTextColumns()){
        myMap[c] = [this](const std::string& in) {
            test("text series column ");
        };
    }

    // Handle time series columns
    auto timestampCol = configManager.getTimestampColumn();
    myMap[timestampCol] =  [this](const std::string& in) {
        timestampManager.compressTimestamps( std::stoi(in) );
    };

    //Handle position columns
    if(configManager.getContainsPosition()){
        auto latCol  = configManager.getLatColumn();
        auto longCol = configManager.getLongColumn();

        //Pak lat og long sammen i et pair i stedet for at kalde dem separat
        myMap[latCol->col] = [this](const std::string& in) {test("lat column ");};
        myMap[longCol->col] = [this](const std::string& in) {test("long column ");};
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
            auto compressFunction = myMap.find(count);
            compressFunction->second(word); //second() contains the lambda function responsible for calling further
                                            //compression methods
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
//    int a, b;
//    auto res = timestampManager.calcIndexRangeFromTimestamps(1645153465,1645311865, a,b);
//
//    std::cout << a << std::endl;
}