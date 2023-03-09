#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>

class PmcMean {
public:
    int lastTimestamp;
    float minValue;
    float maxValue;
    float sumOfValues;
    size_t length;
    double &error;

    PmcMean(double &error_bound, bool error_absolute);

    int fitValuePmc(float value);

    static std::vector<float> gridPmcMean(float value, int timestamp_count);

    [[nodiscard]] float getBytesPerValue() const;

    PmcMean &operator=(const PmcMean &instance);

private:
    bool isErrorAbsolute;

    int isValueWithinErrorBound(float real_value, float approx_value) const;

    static int equalOrNanPmc(float v1, float v2);
};