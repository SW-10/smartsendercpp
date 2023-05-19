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
    int64_t startTime;
    int64_t endTime;
    float error;
    float bitRate;
    int length;
    int8_t mid;
    int8_t cid;
    int8_t localId;
    bool send = true;
    std::vector<float> values;


};

struct TimeSeriesModelContainer {

    TimeSeriesModelContainer(columnsExtra &timeSeries, int localId, bool adjustable);

    TimeSeriesModelContainer(columns &timeSeries, int localId, bool adjustable);

    TimeSeriesModelContainer(double errorBound, int localId, int globalId, bool adjustable);

    const double errorBound;
    int localId;
    int globalId;
    int startTimestamp;
    Status status;
    Gorilla gorilla;
    PmcMean pmcMean;
    Swing swing;
    CachedValues cachedValues;

    //TimeSeriesModelContainer& operator= (const TimeSeriesModelContainer& t);
    TimeSeriesModelContainer &operator=(const TimeSeriesModelContainer &instance);
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
    explicit ModelManager(TimestampManager &timestampManager);

//std::vector<SelectedModel> selectedModels;
    std::vector<std::vector<SelectedModel>> selectedModels;
    void fitSegment(int id, float value, Node *timestamp);

    ModelManager(std::vector<columns> &timeSeriesConfig,
                 TimestampManager &timestampManager);

    void constructFinishedModels(TimeSeriesModelContainer &finishedSegment,
                                 Node *lastTimestamp);

    static bool shouldConstructModel(TimeSeriesModelContainer &container);

    Node * calculateFlushTimestamp();

    void forceModelFlush(int localId);

    std::vector<TimeSeriesModelContainer> timeSeries;

    void
    resetModelManager(std::vector<columnsExtra> &timeSeriesConfig,
                      std::vector<TimeSeriesModelContainer> &&existingModels);

    int getUnfinishedModelSize(int localId);

private:
    TimestampManager &timestampManager;

    static bool shouldCacheData(TimeSeriesModelContainer &container);

    static SelectedModel selectGorilla(TimeSeriesModelContainer &modelContainer, float bitrate);

    static SelectedModel selectSwing(TimeSeriesModelContainer &modelContainer, float bitRate);

    static SelectedModel selectPmcMean(TimeSeriesModelContainer &modelContainer, float bitRate);

    template <class T>
    void RefreshTimeSeries(T &timeSeriesConfig, bool adjustable = false);

    static void CleanAdjustedModels(TimeSeriesModelContainer &finishedSegment);


};