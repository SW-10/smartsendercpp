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

private:

    int getNextLineInOriginalFile(std::fstream& csvFileStream, std::map<int, std::deque<std::pair<int, float>>>& timeseries);
    void decompressOneModel(Model& m,  std::deque<std::pair<int, float>>& originalValues);
    float bytesToFloat(std::vector<uint8_t> bytes);
    std::vector<float> bytesToFloats(std::vector<uint8_t> bytes);
    float calcActualError(std::deque<std::pair<int, float>> &original, const std::vector<float> &reconstructed,
                          int modelType, float errorbound, int col);
};

