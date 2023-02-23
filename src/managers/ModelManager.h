//
// Created by power on 21-02-2023.
//

#ifndef SMARTSENDERCPP_MODELMANAGER_H
#define SMARTSENDERCPP_MODELMANAGER_H

#include "../models/gorilla.h"
#include "../models/PMCMean.h"
#include "../models/swing.h"
#include "ConfigManager.h"


struct status {
    bool pmcMeanReady = true;
    bool SwingReady = true;
};

struct CachedValues {
    int startTimestamp = NULL;
    std::vector<float> values;
};

struct TimeSeriesModelContainer {
    double errorBound;
    bool errorAbsolute;
    Gorilla gorilla;
    Pmc_mean pmcMean;
    Swing swing;
    status status;
    CachedValues cachedValues;
    TimeSeriesModelContainer(double &errorBound, bool errorAbsolute);
    //TimeSeriesModelContainer& operator= (const TimeSeriesModelContainer& t);
};


class ModelManager {
private:
    std::vector<TimeSeriesModelContainer> timeSeries;
    bool shouldCacheData(TimeSeriesModelContainer &);
public:
    void fitTimeSeriesModels(int id, float value);
    ModelManager(std::vector<columns>& timeSeriesConfig);


    bool constructFinishedModels(TimeSeriesModelContainer &finishedSegment);
};


#endif //SMARTSENDERCPP_MODELMANAGER_H
