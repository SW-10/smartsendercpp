#include "BudgetManager.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int budget) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager) {
    this->budget = budget;
    this->bytesLeft = budget;
}

void BudgetManager::endOfChunkCalculations() {
    //DO STUFF
    for(auto &container : modelManager.timeSeries){
        if(timestampManager.timestampCurrent->data - container.startTimestamp > maxAge){
            modelManager.forceModelFlush(container.localId);
        }
    }
    for(auto &selected : modelManager.selectedModels){
        bool flushAll = true;
        int i;
        for(i = 0; i < selected.size(); i++){
            if (bytesLeft < 22 + selected.at(i).values.size()*4){
                flushAll = false;
                break;
            }
            bytesLeft -= 22 + selected.at(i).values.size()*4;
        }
        if (flushAll){
            selected.clear();
        }
        else if(i > 0){
            selected.erase(selected.begin(), selected.begin()+i);
        }

    }

    bytesLeft += budget;
}


