//
// Created by power on 21-02-2023.
//
#define GORILLA_MAX 50
#include "ModelManager.h"
#include "vector"
TimeSeriesModelContainer::TimeSeriesModelContainer(double &errorBound, bool errorAbsolute) 
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
    this->errorAbsolute = errorAbsolute;
}

/*TimeSeriesModelContainer &TimeSeriesModelContainer::operator=(const TimeSeriesModelContainer& t) {
    pmcMean. = t.pmcMean;
    return *this;
}*/


void ModelManager::fitTimeSeriesModels(int id, float value) {
    TimeSeriesModelContainer& container = timeSeries[id];
    int timestamp = 1000;
    if(container.status.SwingReady){
        container.status.SwingReady = container.swing.fitValueSwing(timestamp, value);
    }
    if(container.status.pmcMeanReady){
        container.status.pmcMeanReady = container.pmcMean.fit_value_pmc(value);
    }
    if(container.gorilla.get_length_gorilla() < GORILLA_MAX){
        container.gorilla.fitValueGorilla(value);
    }
    if(shouldCacheData(container)){
        container.cachedValues.values.emplace_back(value);
        if (container.cachedValues.startTimestamp == NULL){
            container.cachedValues.startTimestamp = timestamp;
        }
    }
}

ModelManager::ModelManager(std::vector<columns> &timeSeriesConfig) {
    for (auto &column : timeSeriesConfig){
        timeSeries.emplace_back(column.error, column.isAbsolute);
    }
}

bool ModelManager::shouldCacheData(TimeSeriesModelContainer& container) {
    return !(container.status.SwingReady &&
    container.status.pmcMeanReady &&
    container.gorilla.get_length_gorilla() < GORILLA_MAX);
}

void ModelManager::constructFinishedModels(TimeSeriesModelContainer& finishedSegment){
    float pmcMeanSize = finishedSegment.pmcMean.getBytesPerValue();
    float swingSize = finishedSegment.swing.getBytesPerValue();
    float gorillaSize = finishedSegment.gorilla.getBytesPerValue();

    if (pmcMeanSize > swingSize && pmcMeanSize > gorillaSize){

    } else if (swingSize > pmcMeanSize && swingSize > gorillaSize){

    } else {
        
    }
    if (finishedSegment.cachedValues.startTimestamp != NULL){
        CachedValues innerCache = std::move(finishedSegment.cachedValues);
        // TODO: MAYBE MOVE
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound, finishedSegment.errorAbsolute);
        for (auto value : finishedSegment.cachedValues.values){

        }
    }

}
