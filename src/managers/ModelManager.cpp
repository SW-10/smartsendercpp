//
// Created by power on 21-02-2023.
//

#include "ModelManager.h"
#include "vector"
TimeSeriesModelContainer::TimeSeriesModelContainer(double errorBound, bool errorAbsolute) :
    pmcMean(errorBound, errorAbsolute),
    swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
}

void ModelManager::fitTimeSeriesModels(int id, float value) {
    //this->timeSeries[0].errorBound
}

ModelManager::ModelManager(std::vector<columns> &timeSeriesConfig) {
    for (auto &column : timeSeriesConfig){
        timeSeries.emplace_back(column.error, column.isAbsolute);
    }
}
