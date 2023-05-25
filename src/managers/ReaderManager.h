#pragma once

#include <fstream>
#include "string"
#include "ConfigManager.h"
#include "ModelManager.h"
#include "TimestampManager.h"
#include "BudgetManager.h"
#include "../utils/OutlierDetector.h"
#ifdef linux
#include "../ArrowFlight.h"
#endif
#include <iostream>
#include <unordered_map>
#include <map>

#include <any>
#include <functional>


class ReaderManager : public IObserver {
private:
    ConfigManager configManager;
    TimestampManager timestampManager;
    OutlierDetector outlierDetector;

public:
    ModelManager modelManager;
    explicit ReaderManager(std::string configFile, Timekeeper &timekeeper);

    void runCompressor();

private:
    BudgetManager budgetManager;
    enum class CompressionType : int {
        VALUES, TIMESTAMP, NONE
    };
    std::fstream csvFileStream;
    std::unordered_map<
            int, // key
            std::tuple<
                    std::function<CompressionType(std::string *, int &lineNum)>,
                    CompressionType,
                    int
            > // value (function, compressionType, count)
    > myMap;

    void decompressModels();
    float bytesToFloat(std::vector<uint8_t> bytes);
    std::vector<float> bytesToFloats(std::vector<uint8_t> bytes);
    void finaliseCompression();
    void Update(const std::string &message_from_subject) override;
    bool newInterval = false;
    #ifndef NDEBUG
    float calcActualError(const std::vector<float>& original, const std::vector<float>& reconstructed, int modelType, float errorbound);
    std::map<int, std::vector<std::pair<int, float>>> timeseries;
    int datasetTotalSize = 0;
    #endif

};