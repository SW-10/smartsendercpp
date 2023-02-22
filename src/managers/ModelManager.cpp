//
// Created by power on 21-02-2023.
//
#define GORILLA_MAX 50
#include "ModelManager.h"
#include "vector"
TimeSeriesModelContainer::TimeSeriesModelContainer(double &errorBound, bool errorAbsolute) 
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
}

void ModelManager::fitTimeSeriesModels(int id, float value) {
    TimeSeriesModelContainer& container = timeSeries[id];
    if(container.status.SwingReady){
        container.status.SwingReady = container.swing.fitValueSwing(1000, value);
    }
    if(container.status.pmcMeanReady){
        container.pmcMean.fit_value_pmc(value);
    }
    if(container.gorilla.get_length_gorilla() < GORILLA_MAX){
        container.gorilla.fitValueGorilla(value);
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
