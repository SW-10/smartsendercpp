#pragma once

#include <fstream>
#include "string"
#include "ConfigManager.h"
#include "ModelManager.h"
#include "DecompressManager.h"
#include "TimestampManager.h"
#include "BudgetManager.h"
#include "../utils/OutlierDetector.h"
#include <deque>
#ifdef linux
#include "../ArrowFlight.h"
#endif
#include <iostream>
#include <unordered_map>
#include <map>

#include <any>
#include <functional>

struct Model;
class ReaderManager : public IObserver {
private:
    ConfigManager configManager;
    TimestampManager timestampManager;
#ifndef PERFORMANCE_TEST
    DecompressManager decompressManager;
#endif
    OutlierDetector outlierDetector;

public:
    ModelManager modelManager;
    explicit ReaderManager(std::string configFile, Timekeeper &timekeeper);

    void runCompressor();
//    void decompressModels();

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
//    int getNextLineInOriginalFile(std::fstream& csvFileStream, std::map<int, std::deque<std::pair<int, float>>>& timeseries);
//    void decompressOneModel(Model& m,  std::deque<std::pair<int, float>>& originalValues);
#ifndef PERFORMANCE_TEST
    void finaliseCompression();
#endif
    void Update(const std::string &message_from_subject) override;
    bool newInterval = false;

    #ifndef PERFORMANCE_TEST
    float actualTotalError = 0;
    int totalPoints = 0;
    int datasetTotalSize = 0;
    std::unordered_map<int, int> totalNum;
    #endif
};