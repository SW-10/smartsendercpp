#include <numeric>
#include <cmath>
#include "BudgetManager.h"
#include "../utils/Huffman.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int &budget, int &maxAge, int *firstTimestampChunk) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager),
                             budget(budget), maxAge(maxAge)
                             {
    this->firstTimestampChunk = firstTimestampChunk;
    this->bytesLeft = budget;
    for (auto &column: configManager.timeseriesCols) {
         storageImpact.emplace_back();
    }
    sizeOfModels = 0;
    sizeOfModels += sizeof(float); // Size of error
    sizeOfModels += sizeof(int)*2; // size of start+end timestamp
    sizeOfModels += sizeof(int8_t)*2; // Size of model id, column id
}

void BudgetManager::endOfChunkCalculations() {

    // Flush if the first timestamp of the current chunk becomes too old
    for(auto &container : modelManager.timeSeries){
        if(timestampManager.timestampCurrent->data - container.startTimestamp > maxAge){
            modelManager.forceModelFlush(container.localId);
        }
    }

    // Run through all models created for all time series in the chunk.
    // If the sum of all models of a time series is within the budget, we can flush all.
    // Otherwise, we'll only flush up til the model that exceeded the budget
    for(auto &selected : modelManager.selectedModels){
        bool flushAll = true;
        int i;
        for(i = 0; i < selected.size(); i++){
            int modelSize = sizeOfModels + selected.at(i).values.size()*4;
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
    if (!timestampManager.localOffsetListToSend.empty()){
        Huffman huffmanLOL;
        huffmanLOL.runHuffmanEncoding(timestampManager.localOffsetListToSend, false);
        huffmanLOL.encodeTree();
        //Temp Used for testing
        //temp += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();
        timestampManager.localOffsetListToSend.clear();

        //FOR REF
        // Huffman-encode Local offset list
        /*Huffman huffmanLOL;
        auto vecLOL = timestampManager.flattenLOL();
    //    std::vector<int> paperExample = {0,1,5,2,1,1,4,-2,
    //                                     0,1,5,2,1,1,1,2,1,1,1,-2,
    //                                     0,3,1,4,1,3,1,-2,
    //                                     0,2,2,3,1,2,1,-2,
    //                                     1,5,1,4,1,-3};
        std::vector<int> paperExample = {60, 5, 5, 1, 55, 1, 60, 4,-3};
        huffmanLOL.runHuffmanEncoding(paperExample, true);
        huffmanLOL.encodeTree();
        MinHeapNode* LOLtreeRoot  = huffmanLOL.decodeTree();
        auto LOL = huffmanLOL.decodeLOL(LOLtreeRoot, huffmanLOL.huffmanBuilder.bytes);
        std::cout << "HUFFMAN SIZE (LOL): " << huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size() << std::endl;

        // Huffman-encode Global offset list
        Huffman huffmanGOL;
        auto vecGOL = timestampManager.flattenGOL();
        huffmanGOL.runHuffmanEncoding(vecGOL, true);
        huffmanGOL.encodeTree();
        MinHeapNode* GOLtreeRoot  = huffmanGOL.decodeTree();
        auto GOL = huffmanGOL.decodeGOL(GOLtreeRoot, huffmanGOL.huffmanBuilder.bytes);
        std::cout << "HUFFMAN SIZE (GOL): " << huffmanGOL.huffmanBuilder.bytes.size() + huffmanGOL.treeBuilder.bytes.size() << std::endl;
    */
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
        double xIntercept = (intercept - configManager.bufferGoal) / (slope * -1);
        if(xIntercept < configManager.budgetLeftRegressionLength){
            if (slope > 0){
                // TODO lower error bounds
            }
            else if (slope < 0){
                //TODO Increase error bounds
            }
        }
        if (xIntercept > configManager.budgetLeftRegressionLength + configManager.chunksToGoal){
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
    //TODO: Implement
}

void BudgetManager::spaceKeeperEmplace(int size, int index){
    if (storageImpact.at(index).size() == configManager.chunksToGoal){
        storageImpact.at(index).erase(storageImpact.at(index).begin());
    }
    storageImpact.at(index).emplace_back(size);
}




