#pragma once

#include "../models/Gorilla.h"
#include "../models/PMCMean.h"
#include "../models/Swing.h"
#include "ConfigManager.h"
#include "TimestampManager.h"
#include "../utils/Utils.h"

enum model_type_id {PMC_MEAN, SWING, GORILLA};

struct Status {
    bool pmcMeanReady = true;
    bool SwingReady = true;
    bool gorillaReady = true;
};

struct CachedValues {
    Node* startTimestamp;
    std::vector<float> values;
};

struct SelectedModel{
    int8_t mid;
    int8_t cid;
    int64_t startTime;
    int64_t endTime;
    std::vector<float> values;
    float error;
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
    std::vector<SelectedModel> selectedModels;
    void fitSegment(int id, float value, Node *timestamp);

    ModelManager(std::vector<columns> &timeSeriesConfig,
                 std::vector<int> &textCols,
                 TimestampManager &timestampManager);

    void constructFinishedModels(TimeSeriesModelContainer &finishedSegment,
                                 Node *lastTimestamp);

    static bool shouldConstructModel(TimeSeriesModelContainer &container);

    void fitTextModels(int localId, const std::string &value);

    bool calculateFlushTimestamp();

    void forceModelFlush(int globalId);

private:
    std::vector<TimeSeriesModelContainer> timeSeries;
    std::vector<TextModelContainer> textModels;
    TimestampManager &timestampManager;

    static SelectedModel selectPmcMean(TimeSeriesModelContainer &modelContainer);
    static SelectedModel selectSwing(TimeSeriesModelContainer &modelContainer);
    static SelectedModel selectGorilla(TimeSeriesModelContainer &modelContainer);

    static bool
    shouldCacheDataBasedOnPmcSwing(TimeSeriesModelContainer &container);

    static bool shouldCacheData(TimeSeriesModelContainer &container);
};