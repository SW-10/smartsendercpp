#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct slopeAndIntercept {
    double slope;
    double intercept;
};

class Swing {
public:
    double &errorBound;
    int firstTimestamp;
    int lastTimestamp;
    double firstValue; // f64 instead of Value to remove casts in fit_value()
    double upperBoundSlope;
    double upperBoundIntercept;
    double lowerBoundSlope;
    double lowerBoundIntercept;
    int length;
    bool errorAbsolute;

    Swing(double &errorBound, bool isErrorAbsolute);

    bool fitValueSwing(long timestamp, double value);

    std::vector<float>
    gridSwing(float min, float max, uint8_t values,
              std::vector<long> timestamps, int timestampCount);

    double getModelFirst();

    double getModelLast();

    [[nodiscard]] float getBytesPerValue() const;

    Swing &operator=(const Swing &instance);

private:

    int isNan(double val);

    int equalOrNAN(double v1, double v2);

    slopeAndIntercept computeSlopeAndIntercept(
            long firstTimestamp,
            double firstValue,
            long finalTimestamp,
            double finalValue);

    slopeAndIntercept
    decodeAndComputeSlopeAndIntercept(long firstTimestamp, long lastTimestamp,
                                      double minValue, double maxValue,
                                      int value);


};