#pragma once

#include <fstream>
#include "string"
#include "ConfigManager.h"
#include "ModelManager.h"
#include "TimestampManager.h"
#include "../ArrowFlight.h"
#include <iostream>
#include <unordered_map>
#include <map>

#include <any>
#include <functional>

class ReaderManager {
public:
    ModelManager modelManager;

    explicit ReaderManager(std::string configFile);

    arrow::Status runCompressor();

private:
    enum class CompressionType : int {
        TEXT, VALUES, TIMESTAMP, POSITION, NONE
    };
    std::fstream csvFileStream;
    ConfigManager configManager;
    TimestampManager timestampManager;
    bool bothLatLongSeen;
    std::unordered_map<
            int, // key
            std::tuple<
                    std::function<CompressionType(std::string *, int &lineNum)>,
                    CompressionType,
                    int
            > // value (function, compressionType, count)
    > myMap;
};