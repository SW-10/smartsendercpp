//
// Created by power on 21-02-2023.
//
#include "ModelManager.h"
#include <iostream>
#include <utility>
#include "vector"

TimeSeriesModelContainer::TimeSeriesModelContainer(double errorBound,
                                                   bool errorAbsolute,
                                                   int localId, int globalId, bool adjustable)
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute),
        errorBound(errorBound){
    this->errorAbsolute = errorAbsolute;
    this->localId = localId;
    this->globalId = globalId;
    this->startTimestamp = 0;
    pmcMean.adjustable = adjustable;
    swing.adjustable = adjustable;
}

TimeSeriesModelContainer::TimeSeriesModelContainer(columns &timeSeries, int localId,
                                                   bool adjustable)
                                                   : pmcMean(timeSeries.error, timeSeries.isAbsolute),
                                                   swing(timeSeries.error, timeSeries.isAbsolute),
                                                   errorBound(timeSeries.error)
{
    this->errorAbsolute = timeSeries.isAbsolute;
    this->localId = localId;
    this->globalId = timeSeries.col;
    this->startTimestamp = 0;
    pmcMean.adjustable = adjustable;
    swing.adjustable = adjustable;

}

TimeSeriesModelContainer::TimeSeriesModelContainer(columnsExtra &timeSeries, int localId,
                                                   bool adjustable)
        : pmcMean(timeSeries.error, timeSeries.isAbsolute), swing(timeSeries.errorSwing, timeSeries.isAbsolute),
          errorBound(errorBound)
                                                   {
    this->errorAbsolute = timeSeries.isAbsolute;
    this->localId = localId;
    this->globalId = timeSeries.col;
    this->startTimestamp = 0;
    pmcMean.adjustable = adjustable;
    swing.adjustable = adjustable;
}

TimeSeriesModelContainer &TimeSeriesModelContainer::operator=(const TimeSeriesModelContainer &instance){

    errorAbsolute = instance.errorAbsolute;
    cachedValues = instance.cachedValues;
    startTimestamp = instance.startTimestamp;
    status = instance.status;
    gorilla = instance.gorilla;
    swing = instance.swing;
    pmcMean = instance.pmcMean;
    return *this;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
void ModelManager::fitSegment(int id, float value, Node *timestamp) {
    TimeSeriesModelContainer &container = timeSeries.at(id);
    if (container.startTimestamp == 0) {
        container.startTimestamp = timestamp->data;
    }
    if(container.status.SwingReady) {
        container.status.SwingReady = container.swing.fitValueSwing(timestamp, value);
        //Swing sets last constructed timestamp internally
    }
    if (container.status.pmcMeanReady) {
        container.status.pmcMeanReady = container.pmcMean.fitValuePmc(value);
        if (container.status.pmcMeanReady) {
            container.pmcMean.lastTimestamp = timestamp;
        }
    }
    if (container.status.gorillaReady){
        container.status.gorillaReady = container.gorilla.fitValueGorilla(value);
        if(container.status.gorillaReady){
            container.gorilla.lastTimestamp = timestamp;
        }
    }
    if(shouldCacheData(container)) {
        if (container.cachedValues.values.empty()){
            container.cachedValues.startTimestamp = timestamp;
        }
        container.cachedValues.values.emplace_back(value);
    }
    if (shouldConstructModel(container)) {
        constructFinishedModels(container, timestamp);
    }
}

#pragma clang diagnostic pop

ModelManager::ModelManager(TimestampManager &timestampManager) : timestampManager(timestampManager) { }

void ModelManager::resetModelManager(std::vector<columnsExtra> &timeSeriesConfig,
                                     std::vector<TimeSeriesModelContainer>&& existingModels) {
    selectedModels.clear();
    timeSeries.clear();
    RefreshTimeSeries(timeSeriesConfig, true);
    timeSeries = existingModels;
//    for (int i = 0; !existingModels.empty(); i++){
//        timeSeries.at(i) = std::move(existingModels.at(0));
//        existingModels.erase(existingModels.begin());
//    }
    int e = 0;
}

template <class T>
void ModelManager::RefreshTimeSeries(T &timeSeriesConfig, bool adjustable){
    int count = 0;
    for (auto &column: timeSeriesConfig) {
        timeSeries.emplace_back(column, count, adjustable);
        selectedModels.emplace_back();
        count++;
    }
}

ModelManager::ModelManager(std::vector<columns> &timeSeriesConfig,
                           TimestampManager &timestampManager)
        : timestampManager(timestampManager) {
    RefreshTimeSeries(timeSeriesConfig);
}

bool ModelManager::shouldCacheData(TimeSeriesModelContainer& container) {
    return !(container.status.SwingReady && container.status.pmcMeanReady && container.status.gorillaReady);
}

bool ModelManager::shouldConstructModel(TimeSeriesModelContainer &container) {
    return !(container.status.pmcMeanReady ||
    container.status.SwingReady ||
    container.status.gorillaReady);
}

SelectedModel ModelManager::selectPmcMean(TimeSeriesModelContainer &modelContainer, float bitRate) {
    SelectedModel model = SelectedModel();
    model.mid = PMC_MEAN;
    model.cid = modelContainer.globalId;
    model.localId = modelContainer.localId;
    model.startTime = modelContainer.startTimestamp;
    model.endTime = modelContainer.pmcMean.lastTimestamp->data;
    model.values.emplace_back((modelContainer.pmcMean.sumOfValues / modelContainer.pmcMean.length));
    if (modelContainer.pmcMean.adjustable){
        model.error = modelContainer.pmcMean.error;
    }
    else {
        model.error = modelContainer.errorBound;
    }
    model.bitRate = bitRate;
    model.length = modelContainer.pmcMean.length;
    return model;
};

SelectedModel ModelManager::selectSwing(TimeSeriesModelContainer &modelContainer, float bitRate) {
    SelectedModel model = SelectedModel();
    float start_value = modelContainer.swing.upperBoundSlope * modelContainer.swing.firstTimestamp +
                        modelContainer.swing
                                .upperBoundIntercept;
    float end_value = modelContainer.swing.lowerBoundSlope * modelContainer.swing.lastTimestamp->data +
                      modelContainer.swing
                              .lowerBoundIntercept;

    if (start_value < end_value) {
        model.values.emplace_back(start_value);
        model.values.emplace_back(end_value);
    } else {
        model.values.emplace_back(end_value);
        model.values.emplace_back(start_value);
    }

    model.mid = SWING;
    model.cid = modelContainer.globalId;
    model.localId = modelContainer.localId;
    model.startTime = modelContainer.startTimestamp;
    model.endTime = modelContainer.swing.lastTimestamp->data;
    model.values.emplace_back((int) (start_value < end_value));
    if (modelContainer.swing.adjustable){
        model.error = modelContainer.swing.errorBound;
    }
    else {
        model.error = modelContainer.errorBound;
    }
    model.bitRate = bitRate;
    model.length = modelContainer.swing.length;
    return model;
}

SelectedModel ModelManager::selectGorilla(TimeSeriesModelContainer &modelContainer, float bitrate) {
    SelectedModel model = SelectedModel();

    model.mid = GORILLA;
    model.cid = modelContainer.globalId;
    model.localId = modelContainer.localId;
    model.startTime = modelContainer.startTimestamp;
    model.endTime = modelContainer.gorilla.lastTimestamp->data;
    for (auto x: modelContainer.gorilla.compressedValues.bytes) {
        model.values.emplace_back(x);
    }
    model.error = modelContainer.errorBound;
    model.bitRate = bitrate;
    model.length = modelContainer.gorilla.length;

    return model;
}

void ModelManager::CleanAdjustedModels(TimeSeriesModelContainer &finishedSegment){
    finishedSegment.swing.errorBound = finishedSegment.errorBound;
    finishedSegment.pmcMean.error = finishedSegment.errorBound;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

void
ModelManager::constructFinishedModels(TimeSeriesModelContainer &finishedSegment,
                                      Node *lastTimestamp) {
    float pmcMeanSize = finishedSegment.pmcMean.getBytesPerValue();
    float swingSize = finishedSegment.swing.getBytesPerValue();
    float gorillaSize = finishedSegment.gorilla.getBytesPerValue();
    Node* lastModelledTimestamp;
    size_t indexToStart = std::min<size_t>(finishedSegment.pmcMean.length,
                                           std::min<size_t>(
                                                   finishedSegment.gorilla.length,
                                                   finishedSegment.swing.length));

    if (pmcMeanSize <= swingSize && pmcMeanSize <= gorillaSize) {
        selectedModels.at(finishedSegment.localId).emplace_back(selectPmcMean(finishedSegment, pmcMeanSize));
        lastModelledTimestamp = finishedSegment.pmcMean.lastTimestamp;
        indexToStart = finishedSegment.pmcMean.length - indexToStart;
    } else if (swingSize <= pmcMeanSize && swingSize <= gorillaSize) {
        selectedModels.at(finishedSegment.localId).emplace_back(selectSwing(finishedSegment, swingSize));
        lastModelledTimestamp = finishedSegment.swing.lastTimestamp;
        indexToStart = finishedSegment.swing.length - indexToStart;
    } else {
        selectedModels.at(finishedSegment.localId).emplace_back(selectGorilla(finishedSegment, gorillaSize));
        lastModelledTimestamp = finishedSegment.gorilla.lastTimestamp;
        indexToStart = finishedSegment.gorilla.length - indexToStart;
    }
    if(finishedSegment.pmcMean.adjustable && finishedSegment.swing.adjustable){
        CleanAdjustedModels(finishedSegment);
    }
    if (!finishedSegment.cachedValues.values.empty() && lastTimestamp != nullptr) {
        CachedValues currentCache = std::move(finishedSegment.cachedValues);
        // TODO: MAYBE MOVE
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound,
                                                   finishedSegment.errorAbsolute,
                                                   finishedSegment.localId,
                                                   finishedSegment.globalId,
                                                   finishedSegment.pmcMean.adjustable && finishedSegment.swing.adjustable);

        // TODO: get last constructed TS, and parse rest TS to fitSegment
        std::vector<Node*> timestampOffsets;
        timestampManager.getTimestampsByGlobalId(
                finishedSegment.globalId,
                lastModelledTimestamp, lastTimestamp, timestampOffsets);
        int count = 0;
        for (size_t i = indexToStart; i < currentCache.values.size(); i++) {
            fitSegment(finishedSegment.localId, currentCache.values.at(i), timestampOffsets.at(count));
            count++;
        }
    } else {
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound,
                                                   finishedSegment.errorAbsolute,
                                                   finishedSegment.localId,
                                                   finishedSegment.globalId,
                                                   finishedSegment.pmcMean.adjustable && finishedSegment.swing.adjustable);
    }
}

#pragma clang diagnostic pop

bool ModelManager::calculateFlushTimestamp() {
    Node* earliestUsedTimestamp = nullptr;
    for (auto &container : timeSeries){
        if (!container.cachedValues.values.empty()){
            if(earliestUsedTimestamp == nullptr || (*container.cachedValues.startTimestamp).data < (*earliestUsedTimestamp).data) {
                earliestUsedTimestamp = container.cachedValues.startTimestamp;
            }
        }
    }
    return earliestUsedTimestamp == nullptr || timestampManager.flushTimestamps(earliestUsedTimestamp);
}

void ModelManager::forceModelFlush(int localId) {
    constructFinishedModels(timeSeries.at(localId), nullptr);
}

int ModelManager::getUnfinishedModelSize(int localId){
    auto unfinishedSegment = timeSeries.at(localId);
    float pmcMeanSize = unfinishedSegment.pmcMean.getBytesPerValue();
    float swingSize = unfinishedSegment.swing.getBytesPerValue();
    float gorillaSize = unfinishedSegment.gorilla.getBytesPerValue();
    if (unfinishedSegment.pmcMean.length == 0){
        return 0;
    }
    else if (pmcMeanSize <= swingSize && pmcMeanSize <= gorillaSize) {
        return 1;
    } else if (swingSize <= pmcMeanSize && swingSize <= gorillaSize) {
        return 3;
    } else {
        return unfinishedSegment.gorilla.compressedValues.bytes.size();
    }
}

TextModelContainer::TextModelContainer(int localId, int globalId) {
    this->localId = localId;
    this->globalId = globalId;
    this->reCheck = true;
}

/*TEST_CASE("Same values return 1"){

  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString("STATUS:OK",timestamps[i]));
  };
  std::vector<int> comparison = {1,1,1,1,1,1,1,1,1,1};

  CHECK_EQ(checks, comparison);

}

TEST_CASE("Different values return 0"){
  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString(("STATUS:" + std::to_string(i)),timestamps[i]));
  };
  std::vector<int> comparison = {1,0,0,0,0,0,0,0,0,0};

  CHECK_EQ(checks, comparison);

}*/