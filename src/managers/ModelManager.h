#pragma once

#include "../models/Gorilla.h"
#include "../models/PMCMean.h"
#include "../models/Swing.h"
#include "ConfigManager.h"
#include "TimestampManager.h"
#include "../utils/Utils.h"

struct Status {
    bool pmcMeanReady = true;
    bool SwingReady = true;
    bool gorillaReady = true;
};

struct CachedValues {
    Node* startTimestamp;
    std::vector<float> values;
};

struct TimeSeriesModelContainer {
    int localId;
    int globalId;
    double errorBound;
    bool errorAbsolute;
    //int startTimestamp;
    Gorilla gorilla;
    PmcMean pmcMean;
    Swing swing;
    Status status;
    CachedValues cachedValues;

    TimeSeriesModelContainer(double &errorBound, bool errorAbsolute,
                             int localId, int globalId);
    //TimeSeriesModelContainer& operator= (const TimeSeriesModelContainer& t);
};

struct TextModelContainer {
    int localId;
    int globalId;
    std::string text;
    bool reCheck;

    TextModelContainer(int localId, int globalId);
};

class ModelManager {
public:
    void fitSegment(int id, float value, Node *timestamp);

    ModelManager(std::vector<columns> &timeSeriesConfig,
                 std::vector<int> &textCols,
                 TimestampManager &timestampManager);

    void constructFinishedModels(TimeSeriesModelContainer &finishedSegment,
                                 Node *lastTimestamp);

    static bool shouldConstructModel(TimeSeriesModelContainer &container);

    void fitTextModels(int localId, const std::string &value);

    bool calculateFlushTimestamp();

private:
    std::vector<TimeSeriesModelContainer> timeSeries;
    std::vector<TextModelContainer> textModels;
    TimestampManager &timestampManager;

    static bool shouldCacheData(TimeSeriesModelContainer &container);
};