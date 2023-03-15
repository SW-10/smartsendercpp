//
// Created by power on 21-02-2023.
//
#define GORILLA_MAX 50

#include "ModelManager.h"
#include <iostream>
#include <utility>
#include "vector"

TimeSeriesModelContainer::TimeSeriesModelContainer(double &errorBound,
                                                   bool errorAbsolute,
                                                   int localId, int globalId)
        : pmcMean(errorBound, errorAbsolute), swing(errorBound, errorAbsolute) {
    this->errorBound = errorBound;
    this->errorAbsolute = errorAbsolute;
    this->localId = localId;
    this->globalId = globalId;
    this->startTimestamp = 0;
}

void ModelManager::fitTextModels(int id, const std::string &value) {
    TextModelContainer &container = textModels[id];
    if (container.reCheck) {
        container.text = value;
        container.reCheck = false;
    } else if (container.text != value) {
        //TODO: Emit text model to MQTT HERE
        container.reCheck = true;
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

void ModelManager::fitSegment(int id, float value, int timestamp) {
    TimeSeriesModelContainer &container = timeSeries[id];
    bool cachedOnce = false;

    if (container.gorilla.length > GORILLA_MAX) {
        cachedOnce = true;
        container.cachedValues.values.emplace_back(value);
        if (container.cachedValues.startTimestamp == 0) {
            container.cachedValues.startTimestamp = timestamp;
        }
    }
    if (container.status.SwingReady) {
        container.status.SwingReady = container.swing.fitValueSwing(timestamp,
                                                                    value);
        //Swing sets last constructed timestamp internally
    }
    if (container.status.pmcMeanReady) {
        container.status.pmcMeanReady = container.pmcMean.fitValuePmc(value);
        if (container.status.pmcMeanReady) {
            container.pmcMean.lastTimestamp = timestamp;
        }
    }
    if (container.gorilla.length <= GORILLA_MAX) {
        container.gorilla.fitValueGorilla(value);
        container.gorilla.lastTimestamp = timestamp;
        //TODO use timestamp index on model creation instead of check each time - Optimization
    }
    if (shouldCacheDataBasedOnPmcSwing(container) && !cachedOnce) {
        container.cachedValues.values.emplace_back(value);
        if (container.cachedValues.values.empty()) {
            container.cachedValues.startTimestamp = timestamp;
        }
    }
    if (shouldConstructModel(container)) {
        constructFinishedModels(container, timestamp);
    }
}

#pragma clang diagnostic pop

ModelManager::ModelManager(std::vector<columns> &timeSeriesConfig,
                           std::vector<int> &textCols,
                           TimestampManager &timestampManager)
        : timestampManager(timestampManager) {
    int count = 0;
    for (auto &column: timeSeriesConfig) {
        timeSeries.emplace_back(column.error, column.isAbsolute, count,
                                column.col);
        count++;
    }
    count = 0;
    for (auto &textColumn: textCols) {
        textModels.emplace_back(count, textColumn);
        count++;
    }
}

bool ModelManager::shouldCacheDataBasedOnPmcSwing(
        TimeSeriesModelContainer &container) {
    return !(container.status.SwingReady && container.status.pmcMeanReady);
}

bool ModelManager::shouldConstructModel(TimeSeriesModelContainer &container) {
    return !(container.status.pmcMeanReady ||
             container.status.SwingReady ||
             container.gorilla.length < GORILLA_MAX);
}

SelectedModel ModelManager::selectPmcMean(TimeSeriesModelContainer &modelContainer) {
    SelectedModel model = SelectedModel();
    model.mid = PMC_MEAN;
    model.cid = modelContainer.globalId;
    model.startTime = modelContainer.startTimestamp;
    model.endTime = modelContainer.pmcMean.lastTimestamp;
    model.values.emplace_back((modelContainer.pmcMean.sumOfValues / modelContainer.pmcMean.length));
    model.error = modelContainer.errorBound;
    return model;
};

SelectedModel ModelManager::selectSwing(TimeSeriesModelContainer &modelContainer) {
    SelectedModel model = SelectedModel();
    float start_value = modelContainer.swing.upperBoundSlope * modelContainer.swing.firstTimestamp +
                        modelContainer.swing
                                .upperBoundIntercept;
    float end_value = modelContainer.swing.lowerBoundSlope * modelContainer.swing.lastTimestamp +
                      modelContainer.swing
                              .lowerBoundIntercept;

    if (start_value < end_value) {
        model.values.emplace_back(start_value);
        model.values.emplace_back(end_value);
    } else {
        model.values.emplace_back(end_value);
        model.values.emplace_back(start_value);
    }

    model.mid = SWING;
    model.cid = modelContainer.globalId;
    model.startTime = modelContainer.startTimestamp;
    model.endTime = modelContainer.swing.lastTimestamp;
    model.values.emplace_back((int) (start_value < end_value));
    model.error = modelContainer.errorBound;
    return model;
}

SelectedModel ModelManager::selectGorilla(TimeSeriesModelContainer &modelContainer) {
    SelectedModel model = SelectedModel();

    model.mid = GORILLA;
    model.cid = modelContainer.globalId;
    model.startTime = modelContainer.startTimestamp;
    model.endTime = modelContainer.gorilla.lastTimestamp;
    for (auto x: modelContainer.gorilla.compressedValues.bytes) {
        model.values.emplace_back(x);
    }
    model.error = modelContainer.errorBound;

    return model;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

void
ModelManager::constructFinishedModels(TimeSeriesModelContainer &finishedSegment,
                                      int lastTimestamp) {
    float pmcMeanSize = finishedSegment.pmcMean.getBytesPerValue();
    float swingSize = finishedSegment.swing.getBytesPerValue();
    float gorillaSize = finishedSegment.gorilla.getBytesPerValue();
    int lastModelledTimestamp;
    size_t indexToStart = std::min<size_t>(finishedSegment.pmcMean.length,
                                           std::min<size_t>(
                                                   finishedSegment.gorilla.length,
                                                   finishedSegment.swing.length));

    if (finishedSegment.startTimestamp == 0) {
        finishedSegment.startTimestamp = timestampManager.allTimestampsReconstructed[timestampManager.latestTimestamps[finishedSegment.globalId].timestampFirst];
    }
    if (pmcMeanSize < swingSize && pmcMeanSize < gorillaSize) {
        selectedModels.emplace_back(selectPmcMean(finishedSegment));
        lastModelledTimestamp = finishedSegment.pmcMean.lastTimestamp;
        indexToStart = finishedSegment.pmcMean.length - indexToStart;
    } else if (swingSize < pmcMeanSize && swingSize < gorillaSize) {
        selectedModels.emplace_back(selectSwing(finishedSegment));
        lastModelledTimestamp = finishedSegment.swing.lastTimestamp;
        indexToStart = finishedSegment.swing.length - indexToStart;
    } else {
        selectedModels.emplace_back(selectGorilla(finishedSegment));
        lastModelledTimestamp = finishedSegment.gorilla.lastTimestamp;
        indexToStart = finishedSegment.gorilla.length - indexToStart;
    }
    if (!finishedSegment.cachedValues.values.empty()) {
        CachedValues currentCache = std::move(finishedSegment.cachedValues);
        // TODO: MAYBE MOVE
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound,
                                                   finishedSegment.errorAbsolute,
                                                   finishedSegment.localId,
                                                   finishedSegment.globalId);
        finishedSegment.startTimestamp = lastTimestamp;

        // TODO: get last constructed TS, and parse rest TS to fitSegment
        std::vector<int> timestamps = timestampManager.getTimestampsByGlobalId(
                finishedSegment.globalId,
                lastModelledTimestamp, lastTimestamp);
        int count = 0;
        for (size_t i = indexToStart; i <
                                      currentCache.values.size(); i++) { // Added +1 to indexToStart. Don't know if this is the right way?
            fitSegment(finishedSegment.localId, currentCache.values[i],
                       timestamps[count]);
            count++;
        }
    } else {
        finishedSegment = TimeSeriesModelContainer(finishedSegment.errorBound,
                                                   finishedSegment.errorAbsolute,
                                                   finishedSegment.localId,
                                                   finishedSegment.globalId);
        finishedSegment.startTimestamp = lastTimestamp;
    }
}

#pragma clang diagnostic pop

TextModelContainer::TextModelContainer(int localId, int globalId) {
    this->localId = localId;
    this->globalId = globalId;
    this->reCheck = true;
}

/*TEST_CASE("Same values return 1"){

  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString("STATUS:OK",timestamps[i]));
  };
  std::vector<int> comparison = {1,1,1,1,1,1,1,1,1,1};

  CHECK_EQ(checks, comparison);

}

TEST_CASE("Different values return 0"){
  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString(("STATUS:" + std::to_string(i)),timestamps[i]));
  };
  std::vector<int> comparison = {1,0,0,0,0,0,0,0,0,0};

  CHECK_EQ(checks, comparison);

}*/