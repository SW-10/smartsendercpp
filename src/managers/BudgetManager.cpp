#include <numeric>
#include <cmath>
#include "BudgetManager.h"
#include "../utils/Huffman.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int &budget, int &maxAge, int *firstTimestampChunk) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager),
                             budget(budget), maxAge(maxAge)
                             {
    //this->budget = budget;
    this->firstTimestampChunk = firstTimestampChunk;
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
    if (!timestampManager.localOffsetListToSend.empty()){
        Huffman huffmanLOL;
        huffmanLOL.runHuffmanEncoding(timestampManager.localOffsetListToSend, false);
        huffmanLOL.encodeTree();
        temp += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();
        std::cout << temp << std::endl;
        //std::cout << "HUFFMAN SIZE (LOL): " << huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size() << std::endl;
        timestampManager.localOffsetListToSend.clear();
    }


    bytesLeft += budget;
    lastBudget.emplace_back(bytesLeft);
    if(lastBudget.size()-1 == configManager.budgetLeftRegressionLength){
        lastBudget.erase(lastBudget.begin());
        const double avgX = 0.5 + (configManager.budgetLeftRegressionLength * 0.5); //= 5.5;
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
        if (xp < 0){
            //TODO: adjust error bounds
        }
        //if ()



        //double xpAngle = atan((intercept)/(1+intercept*0))*180/M_PI;
        int i = 2;

        //if(bytesLeft > configManager.bufferGoal){

            /*double estimatedUsage = slope * (configManager.chunksToGoal + configManager.budgetLeftRegressionLength) + intercept;
            if (estimatedUsage < configManager.bufferGoal){
                //TODO: Adjust error bounds
            }*/
        //}
    }



}


