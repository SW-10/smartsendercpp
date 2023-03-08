//
// Created by power on 21-02-2023.
//
#define GORILLA_MAX 50
#include "ModelManager.h"
#include <iostream>
#include <utility>
#include "vector"

TimeSeriesModelContainer::TimeSeriesModelContainer(double &errorBound, bool errorAbsolute, int localId, int globalId)
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
    this->errorAbsolute = errorAbsolute;
    this->localId = localId;
    this->globalId = globalId;
}

void ModelManager::fitTextModels(int id, const std::string& value){
    TextModelContainer& container = textModels[id];
    if(container.reCheck){
        container.text = value;
        container.reCheck = false;
    }
    else if (container.text != value) {
        //TODO: Emit text model to MQTT HERE
        container.reCheck = true;
    }
}

//Recursive chain call is on purpose
void ModelManager::fitSegment(int id, float value, int timestamp) {
    TimeSeriesModelContainer& container = timeSeries[id];
    bool cachedOnce = false;

    if(container.gorilla.get_length_gorilla() > GORILLA_MAX){
        cachedOnce = true;
        container.cachedValues.values.emplace_back(value);
        if (container.cachedValues.startTimestamp == 0){
            container.cachedValues.startTimestamp = timestamp;
        }
    }
    if(container.status.SwingReady){
        container.status.SwingReady = container.swing.fitValueSwing(timestamp, value);
        //Swing sets last constructed timestamp internally
    }
    if(container.status.pmcMeanReady){
        container.status.pmcMeanReady = container.pmcMean.fit_value_pmc(value);
        if (container.status.pmcMeanReady){
            container.pmcMean.lastTimestamp = timestamp;
        }
    }
    if(container.gorilla.get_length_gorilla() <= GORILLA_MAX){
        container.gorilla.fitValueGorilla(value);
        container.gorilla.lastTimestamp = timestamp;
        //TODO use timestamp index on model creation instead of check each time - Optimization
    }
    if(shouldCacheDataBasedOnPmcSwing(container) && !cachedOnce){
        container.cachedValues.values.emplace_back(value);
        if (container.cachedValues.values.empty()){
            container.cachedValues.startTimestamp = timestamp;
        }
    }
    if(shouldConstructModel(container)){
        constructFinishedModels(container, timestamp);
    }
}

ModelManager::ModelManager(std::vector<columns> &timeSeriesConfig, std::vector<int>& text_cols, TimestampManager& timestampManager) : timestampManager(timestampManager) {
    int count = 0;
    for (auto &column : timeSeriesConfig){
        timeSeries.emplace_back(column.error, column.isAbsolute, count, column.col);
        count++;
    }
    count = 0;
    for (auto &textColumn : text_cols){
        textModels.emplace_back(count, textColumn);
        count++;
    }
}

bool ModelManager::shouldCacheDataBasedOnPmcSwing(TimeSeriesModelContainer& container) {
    return !(container.status.SwingReady && container.status.pmcMeanReady);
}

bool ModelManager::shouldConstructModel(TimeSeriesModelContainer& container){
    return !(container.status.pmcMeanReady ||
    container.status.SwingReady ||
    container.gorilla.get_length_gorilla() < GORILLA_MAX);
}

//Recursive call chain OK
void ModelManager::constructFinishedModels(TimeSeriesModelContainer& finishedSegment, int lastTimestamp){
    float pmcMeanSize = finishedSegment.pmcMean.getBytesPerValue();
    float swingSize = finishedSegment.swing.getBytesPerValue();
    float gorillaSize = finishedSegment.gorilla.getBytesPerValue();
    int lastModelledTimestamp;
    size_t indexToStart = std::min<size_t>(finishedSegment.pmcMean.get_length(), std::min<size_t>(finishedSegment.gorilla.get_length_gorilla(), finishedSegment.swing.getLength()));

    if (pmcMeanSize < swingSize && pmcMeanSize < gorillaSize){
        lastModelledTimestamp = finishedSegment.pmcMean.lastTimestamp;
        indexToStart = finishedSegment.pmcMean.get_length() - indexToStart;
    } else if (swingSize < pmcMeanSize && swingSize < gorillaSize){
        lastModelledTimestamp = finishedSegment.swing.get_last_timestamp();
        indexToStart = finishedSegment.swing.getLength() - indexToStart;
    } else {
        lastModelledTimestamp = finishedSegment.gorilla.lastTimestamp;
        indexToStart = finishedSegment.gorilla.get_length_gorilla() - indexToStart;
    }
    if (!finishedSegment.cachedValues.values.empty()){
        CachedValues currentCache = std::move(finishedSegment.cachedValues);
        // TODO: MAYBE MOVE
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound, finishedSegment.errorAbsolute, finishedSegment.localId, finishedSegment.globalId);

        // TODO: get last constructed TS, and parse rest TS to fitSegment
        std::vector<int> timestamps = timestampManager.getTimestampsByGlobalId(finishedSegment.globalId,
                                                                               lastModelledTimestamp, lastTimestamp);
        int count = 0;
        for (size_t i = indexToStart; i < currentCache.values.size(); i++){ // Har tilføjet '+1' til indexToStart. Ved ikke, om det er rigtigt?
            fitSegment(finishedSegment.localId, currentCache.values[i], timestamps[count]);
            count++;
        }
    }
    else {
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound, finishedSegment.errorAbsolute, finishedSegment.localId, finishedSegment.globalId);
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