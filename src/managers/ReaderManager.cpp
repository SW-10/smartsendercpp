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
        std::cout << "dfasasdasdasdasddasd" << std::endl;

    for(auto &c : *configManager.getTimeSeriesColumns()){
        // myMap.insert(std::make_pair(c.col, test));
        myMap[c.col] = test;
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

        int count = 0;
        while (std::getline(s, word, ',')){
            auto hello = myMap.find(count);
            std::any_cast <void (*) (std::string) > (hello->second) ("hello there");
            row.push_back(word);
            count++;
        }
        
        timestampManager.addTimestamp(configManager.getTimestampColumn());

        break;
    }
}
