#include <numeric>
#include <cmath>
#include <fstream>
#include "BudgetManager.h"
#include "../utils/Huffman.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int &budget, int &maxAge, int *firstTimestampChunk) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager),
                             budget(budget), maxAge(maxAge),
                             adjustingModelManager(timestampManager)
                             {
    this->firstTimestampChunk = firstTimestampChunk;
    this->bytesLeft = budget;
    for (auto &_: configManager.timeseriesCols) {
        tsInformation.emplace_back(_.col);
        outlierCooldown[_.col] = 0;
    }
    sizeOfModels = 0;
    sizeOfModels += sizeof(float); // Size of error
    sizeOfModels += sizeof(int)*2; // size of start+end timestamp
    sizeOfModels += sizeof(int8_t)*2; // Size of model id, column id
}

void BudgetManager::endOfChunkCalculations() {
    for(auto &container : modelManager.timeSeries){

        // Reset error bound to default error bound
        configManager.timeseriesCols.at(container.localId).error = container.errorBound;

        if(timestampManager.timestampCurrent->data - container.startTimestamp > maxAge){
            modelManager.forceModelFlush(container.localId);
        }
    }

    if(!adjustableTimeSeriesConfig.empty()){
        selectAdjustedModels();
        adjustableTimeSeriesConfig.clear();
        adjustableTimeSeries.clear();
    }

    int penaltySender = 0;
    for(auto &selected : modelManager.selectedModels){
        bool flushAll = true;
        int i;
        int toFlush = 0;
        for(i = 0; i < selected.size(); i++){
            int modelSize = sizeOfModels + selected.at(i).values.size()*4;
            if (!selected.at(i).send){
                //TODO: Only flush non sending models when, corresponding adjusted has been send.
                spaceKeeperEmplace(std::make_pair(selected.at(i).length, selected.at(i).bitRate), selected.at(i).localId);
            }
            else if (bytesLeft < modelSize){
                if(flushAll == false){
                    toFlush = i;
                    penaltySender -= modelSize - bytesLeft;
                }
                else {
                    penaltySender -= modelSize;
                    flushAll = false;
                }
            }
            else {
                bytesLeft -= modelSize;
                if (selected.at(i).error == configManager.timeseriesCols.at(selected.at(i).localId).error){
                    spaceKeeperEmplace(std::make_pair(selected.at(i).length, selected.at(i).bitRate), selected.at(i).localId);
                }
            }
        }
        if (flushAll){
            selected.clear();
        }
        else if(toFlush > 0){
            selected.erase(selected.begin(), selected.begin()+i);
        }
    }

    cleanSpaceKeeper();
    if (!timestampManager.localOffsetListToSend.empty()){
        Huffman huffmanLOL;
        huffmanLOL.runHuffmanEncoding(timestampManager.localOffsetListToSend, false);
        huffmanLOL.encodeTree();
        //Temp Used for testing
        //temp += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();
        timestampManager.localOffsetListToSend.clear();
    }
    if(!timestampManager.globalOffsetListToSend.empty()){
        Huffman huffmanLOL;
        huffmanLOL.runHuffmanEncoding(timestampManager.globalOffsetListToSend, false);
        huffmanLOL.encodeTree();
        //tempg += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();
        timestampManager.globalOffsetListToSend.clear();
    }

    lastBudget.emplace_back(bytesLeft + penaltySender);
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
                lowerErrorBounds();
                // TODO lower error bounds
            }
            else if (slope < 0){
                increaseErrorBounds();
            }
        }
        if (xIntercept > configManager.budgetLeftRegressionLength + configManager.chunksToGoal){
            if (slope < 0) {
                lowerErrorBounds();
                // TODO: lower error bounds
            }
            else if (slope > 0) {
                increaseErrorBounds();
            }
        }
    }
    bytesLeft += budget;
    WriteBitToCSV();

}

void BudgetManager::spaceKeeperEmplace(std::pair<int, float> size, int index){
    tsInformation.at(index).storageImpact.emplace_back(size);
}

void BudgetManager::cleanSpaceKeeper(){
    //Calculate weighted average of byte rate over the last x number of datapoints for each time series
    const int max = 300;
    for(auto &timeSeries: tsInformation){
        int numEntries = 0;
        float weightedSum = 0;
        int i;
        for (i = timeSeries.storageImpact.size()-1; i > -1; i--){
            if(!(timeSeries.storageImpact.at(i).first + numEntries < max)){
                timeSeries.storageImpact.at(i).first = max - numEntries;
                numEntries = max;
                weightedSum += timeSeries.storageImpact.at(i).first*timeSeries.storageImpact.at(i).second;
                break;
            } else {
                numEntries += timeSeries.storageImpact.at(i).first;
                weightedSum += timeSeries.storageImpact.at(i).first*timeSeries.storageImpact.at(i).second;
            }
        }
        if (i > 0){
            timeSeries.storageImpact.erase(timeSeries.storageImpact.begin(), timeSeries.storageImpact.begin()+i);
        }
        if(numEntries > 0){
            timeSeries.byteRate = weightedSum / numEntries;
        }
    }
}

void BudgetManager::increaseErrorBounds(){
    std::vector<std::pair<float, std::pair<int,int>>> largestImpacts;
    largestImpacts.reserve(tsInformation.size());

    // Get global IDs of time series that are cooldowning
    std::vector<int> cooldownIDs;
    for(auto iter : outlierCooldown){
        if(iter.second > 0){
            // iter.first is the global ID
            cooldownIDs.push_back(iter.first);
        }
    }

    // Don't add time series that are cooldowning
    bool cooldownerFound = false;;
    for (int i = 0; i < tsInformation.size(); i++){
        cooldownerFound = false;
        for(auto cooldown : cooldownIDs){
            if(tsInformation.at(i).globalId == cooldown){
                cooldownerFound  = true;
                break;
            }
        }

        if(!cooldownerFound){
            largestImpacts.emplace_back(tsInformation.at(i).byteRate, std::make_pair(i, tsInformation.at(i).globalId));
        }
    }

    //Sort time series by byte rate
    std::sort(largestImpacts.begin(), largestImpacts.end(),
              [](std::pair<float, std::pair<int,int>> &left, std::pair<float, std::pair<int,int>> &right){
                  return left.first > right.first;
              });
    int stop = std::min(10, static_cast<int>(largestImpacts.size()-1));
    std::vector<TimeSeriesModelContainer> existingModels;
    existingModels.reserve(stop);
    for(int i = 0; i < stop; i++){
        //Create config instance for adjusted modeManager
        //TODO set error according to config
        adjustableTimeSeriesConfig.emplace_back(largestImpacts.at(i).second.second,5,0);
        //Create map, which are used to map original localID's to adjusted localID's
        adjustableTimeSeries[largestImpacts.at(i).second.first] = i;
        existingModels.emplace_back(modelManager.timeSeries.at(largestImpacts.at(i).second.first));
    }
    adjustingModelManager.resetModelManager(adjustableTimeSeriesConfig, std::move(existingModels));
}

void BudgetManager::lowerErrorBounds(){

    std::vector<std::pair<float, std::pair<int,int>>> smalllestImpacts;
    smalllestImpacts.reserve(tsInformation.size());

    for (int i = 0; i < tsInformation.size(); i++){
        smalllestImpacts.emplace_back(tsInformation.at(i).byteRate, std::make_pair(i, tsInformation.at(i).globalId));
    }

    //Sort time series by byte rate
    std::sort(smalllestImpacts.begin(), smalllestImpacts.end(),
              [](std::pair<float, std::pair<int,int>> &left, std::pair<float, std::pair<int,int>> &right){
                  return left.first < right.first;
              });

    int stop = std::min(10, static_cast<int>(smalllestImpacts.size()-1));

    for(int i = 0; i < stop; i++){

        //Create config instance for adjusted modeManager
        int locID = smalllestImpacts.at(i).second.first;

        modelManager.forceModelFlush(locID);

        // Set error bound to 0
        configManager.timeseriesCols.at(locID).error = 0;

    }

}

void BudgetManager::lowerErrorBounds(int locID){

    modelManager.forceModelFlush(locID);

    // Set error bound to 0
    configManager.timeseriesCols.at(locID).error = 0;
}


void BudgetManager::selectAdjustedModels(){
    std::cout << std::endl << "New Chunky" << std::endl;
    std::vector<adjustedModelSelectionInfo> scores;

    int toSave = configManager.bufferGoal - lastBudget.back();
    scores.reserve(adjustableTimeSeries.size());
    for (auto &map : adjustableTimeSeries){
        adjustingModelManager.forceModelFlush(map.second);
        //Get finished models
        std::vector<SelectedModel> &adjustedModels = adjustingModelManager.selectedModels.at(map.second);
        std::vector<SelectedModel> &originalModels =  modelManager.selectedModels.at(map.first);
        int adjustedModelStart = adjustedModels.front().startTime;
        int adjustedModelSize = 0;
        int originalModelSize = 0;

        int accumulatedError = 0;
        int accumulatedLength = 0;

        for(const SelectedModel& model: adjustedModels){
            adjustedModelSize += sizeOfModels + model.values.size()*4;
            accumulatedError += model.error * model.length;
            accumulatedLength += model.length;
        }
        int weightedAverageError = accumulatedError / accumulatedLength;

        //Get size of original models - But only those which are on the same chunk as adjusted models
        for(const SelectedModel& model: originalModels){
            if (model.startTime >= adjustedModelStart){
                originalModelSize += sizeOfModels + model.values.size()*4;
            }
        }

        //Get size of last unfinished original model
        int sizeUnfinishedModel = modelManager.getUnfinishedModelSize(map.first);
        if (sizeUnfinishedModel != 0){
            originalModelSize += sizeOfModels + sizeUnfinishedModel*4;
        }

        //Calculate saved bytes by adjusting error bound
        int saved = originalModelSize - adjustedModelSize;
        if (saved > 0){
            //emplace score
            scores.emplace_back(map.first,static_cast<float>(saved) / weightedAverageError, saved, adjustedModelStart);
        }
    }
    //Sort by score
    std::sort(scores.begin(), scores.end(),
              [](auto &left, auto &right){
                  return left.score > right.score;
              });
    if (scores.size() > 0){
        for(int i = 0; toSave > 0 && i < scores.size()-1; i++){
            auto scoreEntry = scores.at(i);
            toSave -= scoreEntry.saved;
            //Flush model of original modelManager
            modelManager.forceModelFlush(scoreEntry.localId);
            //Set original models in same time series not to be sent - but only those in same chunk as adjusted models
            //They are not deleted at they still are needed to calculate storage impact
            for (auto &originalModel: modelManager.selectedModels.at(scoreEntry.localId)){
                if (originalModel.startTime >= scoreEntry.adjustmentStart){
                    originalModel.send = false;
                }
            }
            //Append adjusted models til vector, which should be sent
            modelManager.selectedModels.at(scoreEntry.localId).insert(modelManager.selectedModels.at(scoreEntry.localId).end(),
                                                                      adjustingModelManager.selectedModels.at(adjustableTimeSeries[scoreEntry.localId]).begin(),
                                                                      adjustingModelManager.selectedModels.at(adjustableTimeSeries[scoreEntry.localId]).end());
        }
    }
}

void BudgetManager::WriteBitToCSV(){
//    std::ofstream myfile;
//    myfile.open ("example.csv", std::ios_base::app);
//    for (const auto& instance : tsInformation){
//        myfile << instance.byteRate << ",";
//    }
//    myfile << "\n";
//    myfile.close();
}

timeSeriesInformation::timeSeriesInformation(int8_t globalId) {
    this->globalId = globalId;
}


adjustedModelSelectionInfo::adjustedModelSelectionInfo(int localId, float score, int saved, int adjustmentStart) {
    this->localId = localId;
    this->score = score;
    this->saved = saved;
    this->adjustmentStart = adjustmentStart;
}
