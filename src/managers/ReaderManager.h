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
#include <any>
#include <functional>


using pfunc = void (*)(std::string);

class ReaderManager {
public:
    ReaderManager(std::string configFile);
    void runCompressor();
private:
    std::fstream csvFileStream;
    ConfigManager configManager;
    ModelManager modelManager;
    TimestampManager timestampManager;
    std::unordered_map<int,std::any> myMap;

};

void test(std::string word){ std::cout << "hej " << word << std::endl; }

#endif //SMARTSENDERCPP_READERMANAGER_H
