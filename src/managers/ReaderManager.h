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

class ReaderManager {
public:
    explicit ReaderManager(std::string configFile);

    void runCompressor();

private:
    enum class CompressionType : int {
        TEXT, VALUES, TIMESTAMP, POSITION, NONE
    };
    std::fstream csvFileStream;
    ConfigManager configManager;
    ModelManager modelManager;
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