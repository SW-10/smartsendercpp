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
    int startTimestamp = 0;
    std::vector<float> values;
};

struct TimeSeriesModelContainer {
    int localId;
    int globalId;
    double errorBound;
    bool errorAbsolute;
    int startTimestamp;
    Gorilla gorilla;
    PmcMean pmcMean;
    Swing swing;
    Status status;
    CachedValues cachedValues;
    TimeSeriesModelContainer(double &errorBound, bool errorAbsolute, int localId, int globalId);
    //TimeSeriesModelContainer& operator= (const TimeSeriesModelContainer& t);
};

struct TextModelContainer{
    int localId;
    int globalId;
    std::string text;
    bool reCheck;
    TextModelContainer(int localId, int globalId);
};


class ModelManager {
private:
    std::vector<TimeSeriesModelContainer> timeSeries;
    std::vector<TextModelContainer> textModels;
    TimestampManager& timestampManager;
    static bool shouldCacheData(TimeSeriesModelContainer &);
public:
    void fitTimeSeriesModels(int id, float value, int timestamp);
    ModelManager(std::vector<columns>& timeSeriesConfig, std::vector<int>& text_cols, TimestampManager& timestampManager);

    void constructFinishedModels(TimeSeriesModelContainer &finishedSegment, int lastTimestamp);

    static bool shouldConstructModel(TimeSeriesModelContainer &container);

    void fitTextModels(int localId, const std::string &value);
};


#endif //SMARTSENDERCPP_MODELMANAGER_H
