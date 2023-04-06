#pragma once
#include "ModelManager.h"
#include "ConfigManager.h"
#include "TimestampManager.h"

class BudgetManager {
public:
    BudgetManager(ModelManager &modelManager, ConfigManager &configManager, TimestampManager &timestampManager, int budget);
    void endOfChunkCalculations();

private:
    ModelManager &modelManager;
    ConfigManager &configManager;
    TimestampManager &timestampManager;
    int budget;
    int bytesLeft;
    int maxAge = 720000;
};