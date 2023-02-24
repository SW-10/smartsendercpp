//
// Created by power on 21-02-2023.
//
#define GORILLA_MAX 50
#include "ModelManager.h"

#include <utility>
#include "vector"
TimeSeriesModelContainer::TimeSeriesModelContainer(double &errorBound, bool errorAbsolute, int id)
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
    this->errorAbsolute = errorAbsolute;
    this->id = id;
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
        if (container.cachedValues.startTimestamp == NULL){
            container.cachedValues.startTimestamp = timestamp;
        }
    }
    if(shouldConstructModel(container)){
        constructFinishedModels(container);
    }
}

ModelManager::ModelManager(std::vector<columns> &timeSeriesConfig, std::vector<int>& text_cols, TimestampManager& timestampManager) : timestampManager(timestampManager) {
    for (auto &column : timeSeriesConfig){
        timeSeries.emplace_back(column.col, column.error, column.isAbsolute);
    }
    for (auto &textColumn : text_cols){
        textModels.emplace_back(textColumn);
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
void ModelManager::constructFinishedModels(TimeSeriesModelContainer& finishedSegment){
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
    if (finishedSegment.cachedValues.startTimestamp != NULL){
        CachedValues innerCache = std::move(finishedSegment.cachedValues);
        // TODO: MAYBE MOVE
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound, finishedSegment.errorAbsolute, finishedSegment.id);
        // TODO: get last constructed TS, and parse rest TS to fitTimeSeriesModels
        for (auto value : finishedSegment.cachedValues.values){
            fitTimeSeriesModels(finishedSegment.id, value);
        }
    }
}

TextModelContainer::TextModelContainer(int id) {
    this->id = id;
    this->reCheck = true;
}
