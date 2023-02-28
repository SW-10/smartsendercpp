//
// Created by power on 21-02-2023.
//

#ifndef SMARTSENDERCPP_MODELMANAGER_H
#define SMARTSENDERCPP_MODELMANAGER_H

#include "../models/Gorilla.h"
#include "../models/PMCMean.h"
#include "../models/Swing.h"
#include "ConfigManager.h"
#include "TimestampManager.h"


struct Status {
    bool pmcMeanReady = true;
    bool SwingReady = true;
};

struct CachedValues {
    int startTimestamp = NULL;
    std::vector<float> values;
};

struct TimeSeriesModelContainer {
    int id;
    double errorBound;
    bool errorAbsolute;
    int startTimestamp;
    Gorilla gorilla;
    PmcMean pmcMean;
    Swing swing;
    Status status;
    CachedValues cachedValues;
    TimeSeriesModelContainer(double &errorBound, bool errorAbsolute, int id);
    //TimeSeriesModelContainer& operator= (const TimeSeriesModelContainer& t);
};

struct TextModelContainer{
    int id;
    std::string text;
    bool reCheck;
    TextModelContainer(int id);
};


class ModelManager {
private:
    std::vector<TimeSeriesModelContainer> timeSeries;
    std::vector<TextModelContainer> textModels;
    TimestampManager& timestampManager;
    static bool shouldCacheData(TimeSeriesModelContainer &);
public:
    void fitTimeSeriesModels(int id, float value);
    ModelManager(std::vector<columns>& timeSeriesConfig, std::vector<int>& text_cols, TimestampManager& timestampManager);

    void constructFinishedModels(TimeSeriesModelContainer &finishedSegment, int lastTimestamp);

    static bool shouldConstructModel(TimeSeriesModelContainer &container);

    void fitTextModels(int id, const std::string &value);
};


#endif //SMARTSENDERCPP_MODELMANAGER_H
