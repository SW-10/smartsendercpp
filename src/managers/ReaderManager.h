//
// Created by power on 21-02-2023.
//

#ifndef SMARTSENDERCPP_READERMANAGER_H
#define SMARTSENDERCPP_READERMANAGER_H

#include <fstream>
#include "string"
#include "ConfigManager.h"
#include "ModelManager.h"

class ReaderManager {
public:
    ReaderManager(std::string configFile);
    void runCompressor();
private:
    std::fstream csvFileStream;
    ConfigManager configManager;
    ModelManager modelManager;
};


#endif //SMARTSENDERCPP_READERMANAGER_H
