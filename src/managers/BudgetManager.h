#pragma once
#include "ModelManager.h"
#include "ConfigManager.h"
#include "TimestampManager.h"
#include <unordered_map>

struct timeSeriesInformation {
    std::vector<std::pair<int, float>> storageImpact;
    float byteRate = 0;
    int8_t globalId;
    explicit timeSeriesInformation(int8_t globalId);
};

struct adjustedModelSelectionInfo {
    adjustedModelSelectionInfo(int localId, float score, int saved, int adjustmentStart);

    int localId;
    float score;
    int saved;
    int adjustmentStart;
};

class BudgetManager {
public:
    BudgetManager(ModelManager &modelManager, ConfigManager &configManager, TimestampManager &timestampManager,
                  int &budget, int &maxAge);

    void endOfChunkCalculations();
    void lowerErrorBounds();
    void decreaseErrorBounds(int locID);
    ModelManager adjustingModelManager;
    std::unordered_map<int, int> adjustableTimeSeries;

    std::map<int,int> outlierCooldown;
    const int cooldown = 10;
    int sizeOfModels;

    // ======== Variables for evaluation ========
#ifndef PERFORMANCE_TEST
    int huffmanSizeTotal = 0;
    int arithmeticSizeTotal = 0;
    int modelSizeTotal = 0;
    float weightedSum = 0;
    int totalLength = 0;
    float errorBoundTotal = 0;
    std::unordered_map<int, int> flushed;
    static void writeModelsToCsv(std::vector<SelectedModel> models, std::string name);
#endif

    std::string name;
    bool loweringError;
    bool increasingError;
private:
    ModelManager &modelManager;

    ConfigManager &configManager;
    TimestampManager &timestampManager;
    std::vector<columnsExtra> adjustableTimeSeriesConfig;
    int &budget;
    int bytesLeft;

    int &maxAge;
    std::vector<int> lastBudget;
    std::vector<timeSeriesInformation> tsInformation;
    std::unordered_map<int, std::pair<int, int>> lowerModelLength;
    int lowerErrorBoundEndTimestamp;
    int numberIncreasingAdjustableTimeSeries;
    int numberDecreasingAdjustableTimeSeries;

    void spaceKeeperEmplace(std::pair<int, float> size, int index);

    void increaseErrorBounds();

    void cleanSpaceKeeper();

    void selectAdjustedModels();

    void captureWeightedSumAndLength(std::vector<SelectedModel> models);


};