#pragma once

#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include "ConfigManager.h"
#include "BudgetManager.h"


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

private:
    void decompressOneModel(Model& m,  std::deque<std::pair<int, float>>& originalValues);
    float bytesToFloat(std::vector<uint8_t> bytes);
    std::vector<float> bytesToFloats(std::vector<uint8_t> bytes);
    float calcActualError(std::deque<std::pair<int, float>> &original, const std::vector<float> &reconstructed,
                          int modelType, float errorbound, int col);

    int
    getNextLineInOriginalFile(std::fstream &csvFileStream, std::map<int, std::deque<std::pair<int, float>>> &timeseries);
};

