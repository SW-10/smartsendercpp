#include "Swing.h"
#include "../constants.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#ifndef NDEBUG
#include "../doctest.h"
#include "../utils/Utils.h"

#endif
Swing::Swing(double &errorBound)
        : errorBound(errorBound) {
    firstTimestamp = 0;
    firstValue = 0;
    upperBoundSlope = 0;
    upperBoundIntercept = 0;
    lowerBoundSlope = 0;
    lowerBoundIntercept = 0;
    length = 0;
    adjustable = false;
    maxError = 10;
}

bool Swing::fitValueSwing(Node *timestamp, double value) {
    double maximumDeviation;
    if(adjustable){
        errorBound = maxError;
    }

    maximumDeviation = fabs(value * (errorBound / 100.0));

    if (length == 0) {
        // Line 1 - 2 of Algorithm 1 in the Swing and Slide paper.
        firstTimestamp = timestamp->data;
        lastTimestamp = timestamp;
        firstValue = value;
        length += 1;
        return true;
    } else if (isNan(firstValue) || isNan(value)) {
        // Extend Swing to handle both types of infinity and NAN.
        if (equalOrNAN(firstValue, value)) {
            lastTimestamp = timestamp;
            upperBoundSlope = value;
            upperBoundIntercept = value;
            lowerBoundSlope = value;
            lowerBoundIntercept = value;
            length += 1;
            return true;
        } else {
            return false;
        }
    } else if (length == 1) {
        // Line 3 of Algorithm 1 in the Swing and Slide paper.
        lastTimestamp = timestamp;
        struct slopeAndIntercept slopes = computeSlopeAndIntercept(
                firstTimestamp,
                firstValue,
                timestamp->data,
                value + maximumDeviation
        );

        upperBoundSlope = slopes.slope;
        upperBoundIntercept = slopes.intercept;

        slopes = computeSlopeAndIntercept(
                firstTimestamp,
                firstValue,
                timestamp->data,
                value - maximumDeviation
        );

        lowerBoundSlope = slopes.slope;
        lowerBoundIntercept = slopes.intercept;
        length += 1;
        return true;
    } else {
        // Line 6 of Algorithm 1 in the Swing and Slide paper.
        double upperBoundApproximateValue =
                upperBoundSlope * timestamp->data + upperBoundIntercept;
        double lowerBoundApproximateValue =
                lowerBoundSlope * timestamp->data + lowerBoundIntercept;
        if (upperBoundApproximateValue + maximumDeviation < value){
            if(adjustable){
                double requiredMaximumDeviation = value - upperBoundApproximateValue;
                double newErrorBound = fabs(100 * requiredMaximumDeviation / value);
                if(newErrorBound < maxError){
                    errorBound = newErrorBound;
                    length += 1;
                    return true;
                }
            }
            return false;
        }
        else if(lowerBoundApproximateValue - maximumDeviation > value){
            if(adjustable){
                double requiredMaximumDeviation = value - lowerBoundApproximateValue;
                double newErrorBound = fabs(100 * requiredMaximumDeviation / value);
                if(newErrorBound < maxError){
                    errorBound = newErrorBound;
                    length += 1;
                    return true;
                }
            }
            return false;
        } else {
            lastTimestamp = timestamp;

            // Line 17 of Algorithm 1 in the Swing and Slide paper.
            if (upperBoundApproximateValue - maximumDeviation > value) {
                struct slopeAndIntercept slopes =
                        computeSlopeAndIntercept(
                                firstTimestamp,
                                firstValue,
                                timestamp->data,
                                value + maximumDeviation
                        );
                upperBoundSlope = slopes.slope;
                upperBoundIntercept = slopes.intercept;
            }

            // Line 15 of Algorithm 1 in the Swing and Slide paper.
            if (lowerBoundApproximateValue + maximumDeviation < value) {
                struct slopeAndIntercept slopes =
                        computeSlopeAndIntercept(
                                firstTimestamp,
                                firstValue,
                                timestamp->data,
                                value - maximumDeviation
                        );
                lowerBoundSlope = slopes.slope;
                lowerBoundIntercept = slopes.intercept;
            }
            length += 1;
            return true;
        }
    }
}

slopeAndIntercept Swing::computeSlopeAndIntercept(
        long firstTimestamp,
        double firstValue,
        long finalTimestamp,
        double finalValue) {
    // An if expression is used as it seems that no values can be assigned to
    // firstValue and finalValue so slope * timestamp + intercept = INFINITY
    // or slope * timestamp + intercept = NEG_INFINITY.
    if (!isNan(firstValue) && !isNan(finalValue)) {
        struct slopeAndIntercept sample;
        if (firstValue == finalValue) {
            sample.intercept = firstValue;
            sample.slope = 0.0;
            return sample;
        }
        double slope =
                (finalValue - firstValue) / (finalTimestamp - firstTimestamp);
        double intercept = firstValue - slope * firstTimestamp;
        sample.slope = slope;
        sample.intercept = intercept;
        return sample;
    } else {
        struct slopeAndIntercept sample;
        sample.slope = firstValue;
        sample.intercept = finalValue;
        return sample;
    }
}

double Swing::getModelFirst() {
    return upperBoundSlope * firstTimestamp + upperBoundIntercept;
}

double Swing::getModelLast() {
    return upperBoundSlope * lastTimestamp->data + upperBoundIntercept;
}

int Swing::isNan(double val) {
    return std::isnan(val) || std::isinf(val);
}

int Swing::equalOrNAN(double v1, double v2) {
    return v1 == v2 || (isNan(v1) && isNan(v2));
}

float Swing::getBytesPerValue() const {
    return static_cast<float>(2 * VALUE_SIZE_IN_BYTES) /
           static_cast<float>(length);
}

slopeAndIntercept
Swing::decodeAndComputeSlopeAndIntercept(long firstTimestamp,
                                         long lastTimestamp, double minValue,
                                         double maxValue,
                                         int value) {
    if (value == 1) {
        return computeSlopeAndIntercept(firstTimestamp, minValue,
                                        lastTimestamp, maxValue);
    } else {
        return computeSlopeAndIntercept(firstTimestamp, maxValue, lastTimestamp,
                                        minValue);
    }
}

std::vector<float>
Swing::gridSwing(float min, float max, uint8_t values,
                 std::vector<long> timestamps, int timestampCount) {
    std::vector<float> result;
    struct slopeAndIntercept slopeAndIntercept = decodeAndComputeSlopeAndIntercept(
            timestamps[0],
            timestamps[timestampCount - 1], min,
            max, values);
    for (int i = 0; i < timestampCount; i++) {
        result.push_back(slopeAndIntercept.slope * timestamps[i] +
                         slopeAndIntercept.intercept);
    }
    return result;
}

Swing &Swing::operator=(const Swing &instance) {
    firstTimestamp = instance.firstTimestamp;
    lastTimestamp = instance.lastTimestamp;
    firstValue = instance.firstValue;
    upperBoundSlope = instance.upperBoundSlope;
    upperBoundIntercept = instance.upperBoundIntercept;
    lowerBoundSlope = instance.lowerBoundSlope;
    lowerBoundIntercept = instance.lowerBoundIntercept;
    length = instance.length;
    lastTimestamp = instance.lastTimestamp;
    return *this;
}

bool float_equal(float a, float b) {
    return (std::fabs(a - b) < 0.00001);
}

#ifndef NDEBUG

TEST_CASE("Swing") {
    /*double error_bound = 0.3;
    Swing p(error_bound);
    // p = p.getSwing(errorBound);
    struct Node* newNode = new Node;
    newNode->data = 1;
    CHECK(p.fitValueSwing(newNode, 1.0) == 1);
    newNode->data = 2;
    CHECK(p.fitValueSwing(newNode, 1.3) == 1);
    newNode->data = 3;
    CHECK(p.fitValueSwing(newNode, 1.24) == 1);
    newNode->data = 4;
    CHECK(p.fitValueSwing(newNode, 1.045) == 1);
    newNode->data = 5;
    CHECK(p.fitValueSwing(newNode, 1.23) == 1);
    newNode->data = 6;
    CHECK(p.fitValueSwing(newNode, 1.54) == 1);
    newNode->data = 7;
    CHECK(p.fitValueSwing(newNode, 1.45) == 1);
    newNode->data = 8;
    CHECK(p.fitValueSwing(newNode, 1.12) == 1);
    newNode->data = 9;
    CHECK(p.fitValueSwing(newNode, 1.12) == 1)      ;

    SUBCASE("Results") {
        CHECK(float_equal(p.errorBound, 0.3f));
        CHECK(float_equal(p.firstTimestamp, 1.0f));
        CHECK(float_equal(p.upperBoundSlope, 0.0525f));
        CHECK(float_equal(p.upperBoundIntercept, 0.9475f));
        CHECK(float_equal(p.lowerBoundSlope, 0.048f));
        CHECK(float_equal(p.lowerBoundIntercept, 0.952f));
    }

    SUBCASE("Swing grid") {

        //Grid
        std::vector<float> vals{1.0, 1.3, 1.24, 1.045, 1.23, 1.54, 1.45, 1.12,
                                1.12};
        std::vector<long> timestamps{1, 2, 3, 4, 5, 6, 7, 8, 9};

        //Get y-coordinate for first and last point
        auto y_first = p.getModelFirst();
        auto y_last = p.getModelLast();

        CHECK(y_first == 1);
        CHECK(float_equal(y_last, 1.42f));

        auto res = p.gridSwing(y_first, y_last, 1, timestamps,
                               timestamps.size());
        bool equal = true;
        for (int i = 0; i < vals.size(); i++) {
            if (std::fabs(vals[i] - res[i]) > error_bound) {
                equal = false;
            }
        }

        CHECK(equal == true);
    }*/
}

TEST_CASE("Not all values fit") {
    /*double error_bound = 0.2;
    Swing p(error_bound);
    // p = p.getSwing(0.2); //lower error bounds ensures that not all values fit

    struct Node* newNode = new Node;
    newNode->data = 1;
    CHECK(p.fitValueSwing(newNode, 1.0) == 1);
    newNode->data = 2;
    CHECK(p.fitValueSwing(newNode, 1.3) == 1);
    newNode->data = 3;
    CHECK(p.fitValueSwing(newNode, 1.24) == 1);
    newNode->data = 4;
    CHECK(p.fitValueSwing(newNode, 1.045) == 0);
    newNode->data = 5;
    CHECK(p.fitValueSwing(newNode, 1.23) == 1);
    newNode->data = 6;
    CHECK(p.fitValueSwing(newNode, 1.54) == 1);
    newNode->data = 7;
    CHECK(p.fitValueSwing(newNode, 1.45) == 1);
    newNode->data = 8;
    CHECK(p.fitValueSwing(newNode, 1.12) == 0);
    newNode->data = 9;
    CHECK(p.fitValueSwing(newNode, 1.12) == 0);
     */
}
#endif

