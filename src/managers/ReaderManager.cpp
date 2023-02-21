#include <sstream>
#include "ReaderManager.h"

ReaderManager::ReaderManager(std::string configFile)
        : configManager(configFile), modelManager(*configManager.getTimeSeriesColumns()) {
    this->csvFileStream.open(this->configManager.getInputFile(), std::ios::in);
}

void ReaderManager::runCompressor() {
    while(!this->csvFileStream.eof()){
        std::string line;
        std::getline(this->csvFileStream, line);
        std::stringstream s(line);
    }
}
