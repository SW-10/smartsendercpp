#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include "../utils/Utils.h"

class PmcMean {
public:
    size_t length;
    Node* lastTimestamp;
    double &error;
    float minValue;
    float maxValue;
    float sumOfValues;
    float maxError;
    bool adjustable;

    explicit PmcMean(double &errorBound);

    int fitValuePmc(float value);

    static std::vector<float> gridPmcMean(float value, int timestampCount);

    [[nodiscard]] float getBytesPerValue() const;

    PmcMean &operator=(const PmcMean &instance);

private:

    int isValueWithinErrorBound(float realValue, float approxValue) const;

    static int equalOrNanPmc(float v1, float v2);
public:

};