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

    for(int i = 0; i < configManager.getNumberOfCols(); i ++){
        myMap[i] = [this](std::string in) {};
    }

    // TODO: Change all test-functions
    for(auto &c : *configManager.getTimeSeriesColumns()){
        myMap[c.col] = [this](std::string in) {test("time series column ");};
    }

    for(const auto &c : configManager.getTextColumns()){
        myMap[c] = [this](std::string in) {test("text series column ");};
    }

    auto timestampCol = configManager.getTimestampColumn();

    // Void pointer is necessary as all lambdas need to have the same signature
    // The void pointer is cast to an integer as 'compressTimestamps' is called
    myMap[timestampCol] =  [this](std::string in) {timestampManager.addTimestamp( std::stoi(in) );};

    if(configManager.getContainsPosition()){
        auto latCol  = configManager.getLatColumn();
        auto longCol = configManager.getLongColumn();

        //Pak lat og long sammen i et pair i stedet for at kalde dem separat
        myMap[latCol->col] = [this](std::string in) {test("lat column ");};
        myMap[longCol->col] = [this](std::string in) {test("long column ");};
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
            auto hello = myMap.find(count); 
            hello->second(word); //second() = the lambda function

            row.push_back(word);
            count++;
        }
        
        // timestampManager.addTimestamp(configManager.getTimestampColumn());

        // break;
    }
}