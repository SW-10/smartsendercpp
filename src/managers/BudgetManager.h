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
    int huffmanSizeTotal = 0;
    int modelSizeTotal = 0;

    static void writeModelsToCsv(std::vector<SelectedModel> models);

private:
    ModelManager &modelManager;

    ConfigManager &configManager;
    TimestampManager &timestampManager;
    std::vector<columnsExtra> adjustableTimeSeriesConfig;
    int &budget;
    int bytesLeft;
    bool loweringError;
    bool increasingError;
    int &maxAge;
    std::vector<int> lastBudget;
    int temp = 0;
    int modelspace = 0;
    //std::vector<std::pair<float ,std::vector<std::pair<int, float>>>> storageImpact;
    std::vector<timeSeriesInformation> tsInformation;
    std::unordered_map<int, std::pair<int, int>> lowerModelLength;
    int lowerErrorBoundEndTimestamp;
    int numberIncreasingAdjustableTimeSeries;
    int numberDecreasingAdjustableTimeSeries;

    void spaceKeeperEmplace(std::pair<int, float> size, int index);

    void increaseErrorBounds();

    void cleanSpaceKeeper();

    void errorBoundStats(double error, int index);

    void selectAdjustedModels();


};