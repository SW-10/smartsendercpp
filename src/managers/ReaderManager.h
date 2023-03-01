//
// Created by power on 21-02-2023.
//

#ifndef SMARTSENDERCPP_READERMANAGER_H
#define SMARTSENDERCPP_READERMANAGER_H

#include <fstream>
#include "string"
#include "ConfigManager.h"
#include "ModelManager.h"
#include "TimestampManager.h"
#include <iostream>
#include <unordered_map>
#include <map>

#include <any>
#include <functional>


using pfunc = void (*)(std::string);

class ReaderManager {
public:
    ReaderManager(std::string configFile);
    void runCompressor();
private:
    int textCount, valuesCount, timestampCount, positionCount;
    enum class CompressionType : int { TEXT, VALUES, TIMESTAMP, POSITION, NONE};
    std::fstream csvFileStream;
    ConfigManager configManager;
    ModelManager modelManager;
    TimestampManager timestampManager;
    bool bothLatLongSeen;
    std::unordered_map<
        int, // key
        std::tuple<
                std::function<CompressionType(std::string*, int &lineNum)>,
                CompressionType,
                int
                > // value (function, compressiontype, count)
    > myMap;

    void test(std::string word){  }
};


#endif //SMARTSENDERCPP_READERMANAGER_H
