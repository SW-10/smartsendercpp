#include "PMCMean.h"
#include "../constants.h"
#include <cmath>
#include <cstdio>
#include <iostream>
#ifndef NDEBUG
#include "../doctest.h"
#endif
PmcMean::PmcMean(double &errorBound, bool errorAbsolute)
        : error(errorBound) {
    error = errorBound;
    minValue = NAN;
    maxValue = NAN;
    sumOfValues = 0;
    length = 0;
    isErrorAbsolute = errorAbsolute;
}

int PmcMean::fitValuePmc(float value) {
    float nextMinValue = minValue < value ? minValue : value;
    float nextMaxValue = maxValue > value ? maxValue : value;
    float nextSumOfValues = sumOfValues + value;
    size_t nextLength = length + 1;
    float average = (nextSumOfValues / nextLength);

    if (isValueWithinErrorBound(nextMinValue, average) &&
        isValueWithinErrorBound(nextMaxValue, average)) {
        minValue = nextMinValue;
        maxValue = nextMaxValue;
        sumOfValues = nextSumOfValues;
        length = nextLength;
        return 1;
    } else {
        return 0;
    }
}

int
PmcMean::isValueWithinErrorBound(float realValue, float approxValue) const {
    if (equalOrNanPmc(realValue, approxValue)) {
        return 1;
    } else {
        float difference = realValue - approxValue;
        float result = fabsf(difference / realValue);
        if (isErrorAbsolute) {  // check if relative or absolute error
            return fabsf(difference) <= error;
        }
        return (result * 100) <= error;
    }
}

float PmcMean::getBytesPerValue() const {
    return static_cast<float>(VALUE_SIZE_IN_BYTES) / static_cast<float>(length);
}


int PmcMean::equalOrNanPmc(float v1, float v2) {
    return v1 == v2 || (std::isnan(v1) && std::isnan(v2));
}

std::vector<float> PmcMean::gridPmcMean(float value, int timestampCount) {
    std::vector<float> result;
    for (int i = 0; i < timestampCount; i++) {
        result.push_back(value);
    }

    return result;
}

PmcMean &PmcMean::operator=(const PmcMean &instance) {
    minValue = instance.minValue;
    maxValue = instance.maxValue;
    sumOfValues = instance.sumOfValues;
    length = instance.length;
    return *this;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"

#ifndef NDEBUG

TEST_CASE("All values fit") {
    double error = 0.5;
    PmcMean p(error, true);
    p.fitValuePmc(1.0);
    p.fitValuePmc(1.3);
    p.fitValuePmc(1.24);
    p.fitValuePmc(1.045);

    CHECK(p.error == 0.5);
    CHECK(p.length == 4);
    CHECK(p.maxValue == 1.3f);
    CHECK(p.minValue == 1.0f);
    CHECK(p.sumOfValues == 4.585f);

    p.fitValuePmc(0.9);
    p.fitValuePmc(1.54);
    p.fitValuePmc(1.45);
    p.fitValuePmc(1.12);
    p.fitValuePmc(1.12);

    CHECK(p.error == 0.5);
    CHECK(p.length == 9);
    CHECK(p.maxValue == 1.54f);
    CHECK(p.minValue == 0.9f);
    CHECK(p.sumOfValues == 10.715f);
}

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"

TEST_CASE("Not all values fit") {
    double error = 0.2;
    PmcMean p(error, true);

    CHECK(p.fitValuePmc(1.0) == 1);
    CHECK(p.fitValuePmc(1.3) == 1);
    CHECK(p.fitValuePmc(1.24) == 1);
    CHECK(p.fitValuePmc(1.045) == 1);
    CHECK(p.fitValuePmc(0.9) == 0);
    CHECK(p.fitValuePmc(1.54) == 0);
    CHECK(p.fitValuePmc(1.45) == 0);
    CHECK(p.fitValuePmc(1.12) == 1);
    CHECK(p.fitValuePmc(1.12) == 1);
}

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"

TEST_CASE("Grid") {

    //Grid
    double errorBound = 1;
    PmcMean p(errorBound, true);

    std::vector<float> vals{1.0, 1.3, 1.24, 1.045, 0.9, 1.54, 1.45, 1.12, 1.12};
    for (auto v: vals) {
        p.fitValuePmc(v);
    }

    auto res = p.gridPmcMean(p.sumOfValues / p.length, vals.size());
    bool equal = true;
    for (int i = 0; i < vals.size(); i++) {
        if (std::fabs(vals[i] - res[i]) > errorBound) {
            equal = false;
        }
    }

    CHECK(equal == true);
}
#endif

#pragma clang diagnostic pop

