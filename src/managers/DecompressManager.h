#pragma once

#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include "ConfigManager.h"
#include "BudgetManager.h"
#include <algorithm>


struct Model{
    int start;
    int end;
    float errorBound;
    int length;
    int MID;
    int CID;
    std::vector<uint8_t> values;
};

struct ModelError {
    float accErrorBound = 0;
    float accError = 0;
    int totalValues = 0;
    float avgErrorBound = 0;
    float avgError = 0;
    int cid;
    int numOutlier;

};

struct datapoint {

    datapoint(int timestamp, float value, bool outlier);

    int timestamp;
    float value;
    bool important;
};


class DecompressManager {
public:
    ConfigManager& configManager;
    BudgetManager& budgetManager;
    DecompressManager(ConfigManager& configManager, BudgetManager& budgetManager)
        : configManager(configManager), budgetManager(budgetManager){

    }
    void decompressModels();
    float actualTotalError = 0;
    int totalPoints = 0;
    std::map<int, ModelError> columnsError;
    float errorBoundImportant = 0;
    float errorImportant = 0;
    int numImportant = 0;
    float errorBoundNotImportant = 0;
    float errorNotImportant = 0;
    int numNotImportant = 0;
    std::vector<ModelError> columns;

private:
    void decompressOneModel(Model& m, std::deque<datapoint> &originalValues);
    float bytesToFloat(std::vector<uint8_t> bytes);
    std::vector<float> bytesToFloats(std::vector<uint8_t> bytes);
    float calcActualError(std::deque<datapoint> &original, const std::vector<float> &reconstructed,
                          int modelType, float errorbound, int col);

    int
    getNextLineInOriginalFile(std::fstream &csvFileStream, std::map<int, std::deque<datapoint>> &timeseries);
    std::fstream outlier;


};

