//
// Created by power on 21-02-2023.
//
#define GORILLA_MAX 50
#include "ModelManager.h"

#include <utility>
#include "vector"
TimeSeriesModelContainer::TimeSeriesModelContainer(double &errorBound, bool errorAbsolute, int localId, int globalId)
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
    this->errorAbsolute = errorAbsolute;
    this->localId = localId;
    this->globalId = globalId;
}

/*TimeSeriesModelContainer &TimeSeriesModelContainer::operator=(const TimeSeriesModelContainer& t) {
    pmcMean. = t.pmcMean;
    return *this;
}*/
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

void ModelManager::fitTimeSeriesModels(int id, float value) {
    TimeSeriesModelContainer& container = timeSeries[id];
    int timestamp = 1000;
    if(container.status.SwingReady){
        container.status.SwingReady = container.swing.fitValueSwing(timestamp, value);
    }
    if(container.status.pmcMeanReady){
        container.status.pmcMeanReady = container.pmcMean.fit_value_pmc(value);
        //TODO use timestamp index on model creation instead of check each time - Optimization
        if (!container.status.pmcMeanReady){
            container.pmcMean.lastTimestamp = timestamp;
        }
    }
    if(container.gorilla.get_length_gorilla() < GORILLA_MAX){
        container.gorilla.fitValueGorilla(value);
        //TODO use timestamp index on model creation instead of check each time - Optimization
        if (container.gorilla.get_length_gorilla() < GORILLA_MAX){
            container.gorilla.lastTimestamp = timestamp;
        }
    }
    if(shouldCacheData(container)){
        container.cachedValues.values.emplace_back(value);
        if (container.cachedValues.startTimestamp == 0){
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

bool ModelManager::shouldCacheData(TimeSeriesModelContainer& container) {
    return !(container.status.SwingReady &&
    container.status.pmcMeanReady &&
    container.gorilla.get_length_gorilla() < GORILLA_MAX);
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
    int lastModelledTimestamp = 0;

    if (pmcMeanSize > swingSize && pmcMeanSize > gorillaSize){
        lastModelledTimestamp = finishedSegment.pmcMean.lastTimestamp;
    } else if (swingSize > pmcMeanSize && swingSize > gorillaSize){
        lastModelledTimestamp = finishedSegment.swing.get_last_timestamp();
    } else {
        lastModelledTimestamp = finishedSegment.gorilla.lastTimestamp;
    }
    if (finishedSegment.cachedValues.startTimestamp != 0){
        CachedValues innerCache = std::move(finishedSegment.cachedValues);
        // TODO: MAYBE MOVE
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound, finishedSegment.errorAbsolute, finishedSegment.localId, finishedSegment.globalId);
        // TODO: get last constructed TS, and parse rest TS to fitTimeSeriesModels
        int startIndex = 0, endIndex = 0;
//        timestampManager.calcIndexRangeFromTimestamps(lastModelledTimestamp, lastTimestamp, startIndex, endIndex);
        std::vector<int> stamps;
        //timestampManager.getTimestampFromIndex()
        for (auto value : finishedSegment.cachedValues.values){
            fitTimeSeriesModels(finishedSegment.localId, value);
        }
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