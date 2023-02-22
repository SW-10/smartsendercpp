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

struct TimeSeriesModelContainer {
    double errorBound;
    Gorilla gorilla;
    Pmc_mean pmcMean;
    Swing swing;
    status status;
    TimeSeriesModelContainer(double &errorBound, bool errorAbsolute);
};


class ModelManager {
private:
    std::vector<TimeSeriesModelContainer> timeSeries;
public:
    void fitTimeSeriesModels(int id, float value);
    ModelManager(std::vector<columns>& timeSeriesConfig);


};


#endif //SMARTSENDERCPP_MODELMANAGER_H
