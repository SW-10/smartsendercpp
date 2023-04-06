#include "BudgetManager.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int budget) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager) {
    this->budget = budget;
    this->bytesLeft = budget;
}

void BudgetManager::endOfChunkCalculations() {
    //DO STUFF

    bytesLeft += budget;
}


