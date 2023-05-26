#include <numeric>
#include <cmath>
#include <fstream>
#include <cstdint>
#include "BudgetManager.h"
#include "../models/Gorilla.h"
#include "../utils/Huffman.h"


BudgetManager::BudgetManager(ModelManager &modelManager, ConfigManager &configManager,
                             TimestampManager &timestampManager, int &budget, int &maxAge) : modelManager(modelManager),
                             configManager(configManager), timestampManager(timestampManager),
                             budget(budget), maxAge(maxAge),
                             adjustingModelManager(timestampManager)
                             {
    this->bytesLeft = budget;
    for (auto &_: configManager.timeseriesCols) {
        tsInformation.emplace_back(_.col);
        outlierCooldown[_.col] = 0;
    }
    sizeOfModels = 0;
    sizeOfModels += sizeof(float); // Size of error
    sizeOfModels += sizeof(int)*2; // size of start+end timestamp
    sizeOfModels += sizeof(int8_t)*2; // Size of model id, column id
    numberIncreasingAdjustableTimeSeries = std::min(static_cast<int>(modelManager.timeSeries.size()), 60);
    numberDecreasingAdjustableTimeSeries = std::min(static_cast<int>(modelManager.timeSeries.size()), 60);
    increasingError = false;
    loweringError = false;
}

void BudgetManager::endOfChunkCalculations() {
    for(auto &container : modelManager.timeSeries){

        // Reset error bound to default error bound
        // configManager.timeseriesCols.at(container.localId).error = container.errorBound;

        if(timestampManager.timestampCurrent->data - container.startTimestamp > maxAge){
            modelManager.forceModelFlush(container.localId);
        }
    }

    if(!adjustableTimeSeriesConfig.empty()){
        selectAdjustedModels();
        adjustableTimeSeriesConfig.clear();
        adjustableTimeSeries.clear();
        lowerModelLength.clear();
        loweringError = false;
        increasingError = false;
    }

    int penaltySender = 0;
    for(auto &selected : modelManager.selectedModels){
        bool flushAll = true;
        int i;
        int toFlush = 0;
        int Extras = 0;
        for(i = 0; i < selected.size(); i++){
            int modelSize = sizeOfModels + selected.at(i).values.size();
            if (!selected.at(i).send){
                //TODO: Only flush non sending models when, corresponding adjusted has been send.
                spaceKeeperEmplace(std::make_pair(selected.at(i).length, selected.at(i).bitRate), selected.at(i).localId);
            }
            else if (bytesLeft < modelSize){
                if(flushAll == false){
                    penaltySender -= modelSize - bytesLeft;
                }
                else {
                    toFlush = i;
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
            #ifndef NDEBUG
            std::vector<SelectedModel> models;

            for(auto const &s : selected){
                if(s.send){
                    modelSizeTotal += sizeOfModels;
                    modelSizeTotal += s.values.size();
                    models.push_back(s);
                }
            }
            captureWeightedSumAndLength(selected);
            writeModelsToCsv(models);
            #endif
            selected.clear();
        }
        else if(toFlush > 0){
            #ifndef NDEBUG
            std::vector<SelectedModel> models;
            for(int j = 0; j < toFlush; j++){
                if(selected.at(j).send){
                    modelSizeTotal += sizeOfModels;
                    modelSizeTotal += selected.at(j).values.size();
                    models.push_back(selected.at(j));
                }
            }
            captureWeightedSumAndLength(selected);
            writeModelsToCsv(models);
            #endif
            selected.erase(selected.begin(), selected.begin()+toFlush);
        }
    }

    cleanSpaceKeeper();
    if (!timestampManager.localOffsetListToSend.empty()){
        Huffman huffmanLOL;
        huffmanLOL.runHuffmanEncoding(timestampManager.localOffsetListToSend, false);
        huffmanLOL.encodeTree();

        #ifndef NDEBUG
        // CALC SIZE OF HUFFMAN
        huffmanSizeTotal += huffmanLOL.huffmanBuilder.bytes.size() + huffmanLOL.treeBuilder.bytes.size();
        #endif
        timestampManager.localOffsetListToSend.clear();
    }
    if(!timestampManager.globalOffsetListToSend.empty()){
        Huffman huffmanGOL;
        huffmanGOL.runHuffmanEncoding(timestampManager.globalOffsetListToSend, false);
        huffmanGOL.encodeTree();

        #ifndef NDEBUG
        // CALC SIZE OF HUFFMAN
        huffmanSizeTotal += huffmanGOL.huffmanBuilder.bytes.size() + huffmanGOL.treeBuilder.bytes.size();
        #endif
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
            }
            else if (slope < 0){
                increaseErrorBounds();
            }
        }
        if (xIntercept > configManager.budgetLeftRegressionLength + configManager.chunksToGoal){
            if (slope < 0) {
                lowerErrorBounds();
            }
            else if (slope > 0) {
                increaseErrorBounds();
            }
        }
    }
    bytesLeft += budget;

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
    increasingError = true;
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
    int stop = std::min(numberIncreasingAdjustableTimeSeries, static_cast<int>(largestImpacts.size() - 1));
    std::vector<TimeSeriesModelContainer> existingModels;
    existingModels.reserve(stop);
    for(int i = 0; i < stop; i++){
        //Create config instance for adjusted modeManager
        auto &config = configManager.timeseriesCols.at(largestImpacts.at(i).second.first);
        adjustableTimeSeriesConfig.emplace_back(largestImpacts.at(i).second.second, config.error, config.outlierThreshHold, config.maxError);
        //Create map, which are used to map original localID's to adjusted localID's
        adjustableTimeSeries[largestImpacts.at(i).second.first] = i;
        existingModels.emplace_back(modelManager.timeSeries.at(largestImpacts.at(i).second.first));
    }
    adjustingModelManager.resetModelManager(adjustableTimeSeriesConfig, std::move(existingModels));
}

void BudgetManager::lowerErrorBounds(){
    loweringError = true;
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

    int stop = std::min(numberDecreasingAdjustableTimeSeries, static_cast<int>(smalllestImpacts.size()-1));
    auto &ongoingModels = modelManager.timeSeries;

    for(int i = 0; i < stop; i++){

        //Create config instance for adjusted modeManager
        int locID = smalllestImpacts.at(i).second.first;
        auto &config = configManager.timeseriesCols.at(smalllestImpacts.at(i).second.first);
        adjustableTimeSeriesConfig.emplace_back(smalllestImpacts.at(i).second.second, 0, config.outlierThreshHold, config.maxError);
        //modelManager.forceModelFlush(locID);
        adjustableTimeSeries[locID] = i;
        lowerModelLength[locID] = std::make_pair(std::max(ongoingModels.at(locID).swing.length,
                                                          std::max(ongoingModels.at(locID).pmcMean.length, ongoingModels.at(locID).gorilla.length)), ongoingModels.at(locID).startTimestamp);
        // Set error bound to 0
        //configManager.timeseriesCols.at(locID).error = 0;

    }
    lowerErrorBoundEndTimestamp = timestampManager.timestampCurrent->data;
    adjustingModelManager.resetModeManagerLower(adjustableTimeSeriesConfig);


}

void BudgetManager::decreaseErrorBounds(int locID){

    modelManager.forceModelFlush(locID);

    // Set error bound to 0
    configManager.timeseriesCols.at(locID).error = 0;
}


void BudgetManager::selectAdjustedModels(){

    if (increasingError){
        std::vector<adjustedModelSelectionInfo> scores;
        int toSave = configManager.bufferGoal - lastBudget.back();
        scores.reserve(adjustableTimeSeries.size());
        for (auto &map : adjustableTimeSeries){
            adjustingModelManager.forceModelFlush(map.second);
            //Get finished models
            std::vector<SelectedModel> &adjustedModels = adjustingModelManager.selectedModels.at(map.second);
            std::vector<SelectedModel> &originalModels =  modelManager.selectedModels.at(map.first);

            //TESTINGS
            int lengthAdj = 0;
            int lengthOrg = 0;
            for (auto &adj : adjustedModels){
                lengthAdj += adj.length;
            }
            for (auto &org : originalModels){
                lengthOrg += org.length;
            }

            int adjustedModelStart = adjustedModels.front().startTime;
            int adjustedModelSize = 0;
            int originalModelSize = 0;

            int accumulatedError = 0;
            int accumulatedLength = 0;

            for(const SelectedModel& model: adjustedModels){
                adjustedModelSize += sizeOfModels + model.values.size();
                accumulatedError += model.error * model.length;
                accumulatedLength += model.length;
            }
            int weightedAverageError = accumulatedError / accumulatedLength;

            //Get size of original models - But only those which are on the same chunk as adjusted models
            for(const SelectedModel& model: originalModels){
                if (model.startTime >= adjustedModelStart){
                    originalModelSize += sizeOfModels + model.values.size();
                }
            }

            //Get size of last unfinished original model
            int sizeUnfinishedModel = modelManager.getUnfinishedModelSize(map.first);
            if (sizeUnfinishedModel != 0){
                originalModelSize += sizeOfModels + sizeUnfinishedModel;
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
            int i;
            for(i = 0; toSave > 0 && i < scores.size(); i++){
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
            if (toSave > 0){
                numberIncreasingAdjustableTimeSeries = std::min(static_cast<int>(modelManager.timeSeries.size()), numberIncreasingAdjustableTimeSeries + 1);
            }
            else if(i != scores.size()){
                numberIncreasingAdjustableTimeSeries = std::max(numberIncreasingAdjustableTimeSeries - 1, 1);
            }
        }
    }
    else if (loweringError){
        int toFill = lastBudget.back() - configManager.bufferGoal;
        for (auto &map : adjustableTimeSeries){
            adjustingModelManager.forceModelFlush(map.second);
            std::vector<SelectedModel> &adjustedModels = adjustingModelManager.selectedModels.at(map.second);
            std::vector<SelectedModel> &originalModels =  modelManager.selectedModels.at(map.first);
            int adjustedModelStart = adjustedModels.front().startTime;
            int adjustedModelSize = 0;
            int originalModelSize = 0;
            for(const SelectedModel& model: adjustedModels){
                adjustedModelSize += sizeOfModels + model.values.size();
            }

            //Get size of last unfinished original model
            int sizeUnfinishedModel = modelManager.getUnfinishedModelSize(map.first);
            if (sizeUnfinishedModel != 0){
                originalModelSize += sizeOfModels + sizeUnfinishedModel;
            }

            //Get size of original models - But only those which are on the same chunk as adjusted models
            for(int i = 0; i < originalModels.size(); i++){
                if (i < originalModels.size()-1 &&
                    originalModels.at(i).startTime < adjustedModelStart &&
                    originalModels.at(i+1).startTime >= adjustedModelStart

                ){
                    auto IterModel = originalModels.at(i);
                    int iter = i;
                    int cutOffLength = 0;
                    while (IterModel.startTime != lowerModelLength[map.first].second){
                        iter--;
                        cutOffLength += IterModel.length;
                        IterModel = originalModels.at(iter);
                    }
                    auto &model = originalModels.at(i);
                    model.endTime = lowerErrorBoundEndTimestamp;
                    if (model.mid == GORILLA){
                        model.values = Gorilla::getNFirstValuesBitstring(lowerModelLength[map.first].first - cutOffLength, model.values);
                    }
                }
                else if (originalModels.at(i).startTime >= adjustedModelStart){
                    originalModelSize += sizeOfModels + originalModels.at(i).values.size();
                    originalModels.at(i).send = false;
                }
            }
            toFill -= adjustedModelSize - originalModelSize;
            modelManager.selectedModels.at(map.first).insert(modelManager.selectedModels.at(map.first).end(),
                                                                      adjustingModelManager.selectedModels.at(adjustableTimeSeries[map.first]).begin(),
                                                                      adjustingModelManager.selectedModels.at(adjustableTimeSeries[map.first]).end());

            if (toFill < 0){
                break;
            }
        }
        if (toFill > 0){
            numberDecreasingAdjustableTimeSeries  = std::min(static_cast<int>(modelManager.timeSeries.size()), numberDecreasingAdjustableTimeSeries -1);
        }
        else {
            numberDecreasingAdjustableTimeSeries = std::max(numberDecreasingAdjustableTimeSeries + 1, 1);
        }
    }
}

void BudgetManager::captureWeightedSumAndLength(std::vector<SelectedModel> models){
      for (const auto& model : models){
        weightedSum += model.length * model.error;
        totalLength += model.length;
      }
};

void BudgetManager::writeModelsToCsv(std::vector<SelectedModel> models){
    std::ofstream myfile;
    myfile.open ("../models.csv", std::ios_base::app);
    for (const auto& model : models){
        int mid = static_cast<int>(model.mid);
        int cid = static_cast<int>(model.cid);
        myfile <<
        model.startTime << "," <<
        model.endTime   << "," <<
        model.error     << "," <<
        model.length    << "," <<
        mid        << "," <<
        cid       << ",";

        for(const auto val : model.values){
            myfile << (int)(val) << "-";
        }
        myfile << std::endl;
    }
    myfile.close();
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
