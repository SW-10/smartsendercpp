#include <numeric>
#include <cmath>
#include "BudgetManager.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int &budget, int &maxAge, int *firstTimestampChunk) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager),
                             budget(budget), maxAge(maxAge)
                             {
    //this->budget = budget;
    this->firstTimestampChunk = firstTimestampChunk;
    this->bytesLeft = budget;
     for (auto &column: configManager.timeseriesCols) {
         storageImpact.emplace_back();
     }
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
            int modelSize = 22 + selected.at(i).values.size()*4;
            if (bytesLeft < modelSize){
                flushAll = false;
                break;
            }
            bytesLeft -= modelSize;
            spaceKeeperEmplace(modelSize, selected.at(i).cid);
        }
        if (flushAll){
            selected.clear();
        }
        else if(i > 0){
            selected.erase(selected.begin(), selected.begin()+i);
        }

    }

    bytesLeft += budget;
    lastBudget.emplace_back(bytesLeft);
    if(lastBudget.size()-1 == configManager.budgetLeftRegressionLength){
        lastBudget.erase(lastBudget.begin());
        const double avgX = 0.5 + (configManager.budgetLeftRegressionLength * 0.5);
        const double avgY = std::reduce(lastBudget.begin(), lastBudget.end(), 0.0) / lastBudget.size();
        double ba = 0;
        double bb = 0;
        for (int i = 0; i < lastBudget.size(); i++){
            ba += (i+1-avgX)*(lastBudget.at(i)-avgY);
            bb += std::pow((i+1-avgX),2);
        }
        double slope = ba / bb;
        double intercept = avgY - (slope * avgX);

        // Intercept of regression function and buffer goal
        double xp = (intercept - configManager.bufferGoal) / (slope*-1);
        if(xp < configManager.budgetLeftRegressionLength){
            if (slope > 0){
                // TODO lower error bounds
            }
            else if (slope < 0){
                //TODO Increase error bounds
            }
        }
        if (xp > configManager.budgetLeftRegressionLength+configManager.chunksToGoal){
            if (slope < 0) {
                // TODO: lower error bounds
            }
            else if (slope > 0) {
                // TODO: Increase error bounds
            }
        }
    }
}

void BudgetManager::errorBoundAdjuster() {

}

void BudgetManager::spaceKeeperEmplace(int size, int index){
    if (storageImpact.at(index).size() == configManager.chunksToGoal){
        storageImpact.at(index).erase(storageImpact.at(index).begin());
    }
    storageImpact.at(index).emplace_back(size);
}




