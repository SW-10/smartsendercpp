#include "swing.h"
#include "../constants.h"
#include <cstdio>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include "../doctest.h"

Swing::Swing(double &error, bool is_error_absolute)
        : error_bound(error){
  first_timestamp = 0;
  last_timestamp = 0;
  first_value = 0;
  upper_bound_slope = 0;
  upper_bound_intercept = 0;
  lower_bound_slope = 0;
  lower_bound_intercept = 0;
  length = 0;
  error_absolute = is_error_absolute;
}

int Swing::fitValueSwing(long timestamp, double value){
    double maximum_deviation = 0;
    if (error_absolute)  // check if using relative or absolute error bounds
    {
        maximum_deviation = error_bound;
    }
    else
    {
        maximum_deviation = fabs(value * (error_bound / 100.0));

    }
    
    if (length == 0) {
        // Line 1 - 2 of Algorithm 1 in the Swing and Slide paper.
        first_timestamp = timestamp;
        last_timestamp = timestamp;
        first_value = value;
        length += 1;
        return 1;
    } else if (isNan(first_value) || isNan(value)) {
        // Extend Swing to handle both types of infinity and NAN.
        if (equalOrNAN(first_value, value)) {
            last_timestamp = timestamp;
            upper_bound_slope = value;
            upper_bound_intercept = value;
            lower_bound_slope = value;
            lower_bound_intercept = value;
            length += 1;
            return 1;
        } else {
            return 0;
        }
    } else if (length == 1) {
        // Line 3 of Algorithm 1 in the Swing and Slide paper.
        last_timestamp = timestamp;
        struct slopeAndIntercept slopes = compute_slope_and_intercept(
                first_timestamp,
                first_value,
                timestamp,
                value + maximum_deviation
        );

        upper_bound_slope = slopes.slope;
        upper_bound_intercept = slopes.intercept;

        slopes = compute_slope_and_intercept(
                first_timestamp,
                first_value,
                timestamp,
                value - maximum_deviation
        );

        lower_bound_slope = slopes.slope;
        lower_bound_intercept = slopes.intercept;
        length += 1;
        return 1;
    } else {
        // Line 6 of Algorithm 1 in the Swing and Slide paper.
        double upper_bound_approximate_value = upper_bound_slope * timestamp + upper_bound_intercept;
        double lower_bound_approximate_value = lower_bound_slope * timestamp + lower_bound_intercept;

        if (upper_bound_approximate_value + maximum_deviation < value
           || lower_bound_approximate_value - maximum_deviation > value)
        {
            return 0;
        } else {
            last_timestamp = timestamp;

            // Line 17 of Algorithm 1 in the Swing and Slide paper.
            if (upper_bound_approximate_value - maximum_deviation > value) {
                struct slopeAndIntercept slopes =
                        compute_slope_and_intercept(
                                first_timestamp,
                                first_value,
                                timestamp,
                                value + maximum_deviation
                        );
                upper_bound_slope = slopes.slope;
                upper_bound_intercept = slopes.intercept;
            }

            // Line 15 of Algorithm 1 in the Swing and Slide paper.
            if (lower_bound_approximate_value + maximum_deviation < value) {
                struct slopeAndIntercept slopes =
                        compute_slope_and_intercept(
                                first_timestamp,
                                first_value,
                                timestamp,
                                value - maximum_deviation
                        );
                lower_bound_slope = slopes.slope;
                lower_bound_intercept = slopes.intercept;
            }
            length += 1;
            return 1;
        }
    }
}

slopeAndIntercept Swing::compute_slope_and_intercept(
        long first_timestamp,
        double first_value,
        long final_timestamp,
        double final_value) {
    // An if expression is used as it seems that no values can be assigned to
    // first_value and final_value so slope * timestamp + intercept = INFINITY
    // or slope * timestamp + intercept = NEG_INFINITY.
    if (!isNan(first_value) && !isNan(final_value)){
        struct slopeAndIntercept sample;
        if(first_value == final_value){
            sample.intercept = first_value;
            sample.slope = 0.0;
            return sample;
        }
        double slope = (final_value - first_value) / (final_timestamp - first_timestamp);
        double intercept = first_value - slope * first_timestamp;
        sample.slope = slope;
        sample.intercept = intercept;
        return sample;
    } else {
        struct slopeAndIntercept sample;
        sample.slope = first_value;
        sample.intercept = final_value;
        return sample;
    }
}

double Swing::getModelFirst(){
    return upper_bound_slope * first_timestamp + upper_bound_intercept;
}

double Swing::getModelLast(){
    return upper_bound_slope * last_timestamp + upper_bound_intercept;
}

int Swing::isNan(double val){
    return std::isnan(val) || std::isinf(val);
}

int Swing::equalOrNAN(double v1, double v2){
    return v1==v2 || (isNan(v1) && isNan(v2));
}

float Swing::get_bytes_per_value_swing(){
    return (float) (2 * VALUE_SIZE_IN_BYTES) / (float) length;
}
void Swing::get_model_swing(float *arr){
  double first_value = upper_bound_slope * (double) first_timestamp + upper_bound_intercept;

  double last_value = upper_bound_slope * (double) last_timestamp + upper_bound_intercept;

  arr[0] = (float) first_value;
  arr[1] = (float) last_value;
}

size_t Swing::get_length_swing(){
  return length;
}

void Swing::resetSwing(){
    first_timestamp = 0;
    last_timestamp = 0;
    first_value = 0;
    upper_bound_slope = 0;
    upper_bound_intercept = 0;
    lower_bound_slope = 0;
    lower_bound_intercept = 0;
    length = 0;
}

slopeAndIntercept Swing::decode_and_compute_slope_and_intercept(long firstTimestamp, long lastTimestamp, double min_value, double max_value, int value){
    if(value == 1){
        return compute_slope_and_intercept(firstTimestamp, min_value, lastTimestamp, max_value);
    }else{
        return compute_slope_and_intercept(firstTimestamp, max_value, lastTimestamp, min_value);
    }
}

std::vector<float> Swing::gridSwing(float min, float max, uint8_t values, std::vector<long> timestamps,int timestamp_count){
    std::vector<float> result;
    struct slopeAndIntercept slopeAndIntercept = decode_and_compute_slope_and_intercept(timestamps[0], timestamps[timestamp_count-1], min, max, values);
    for(int i = 0; i < timestamp_count; i++){
        result.push_back(slopeAndIntercept.slope * timestamps[i] + slopeAndIntercept.intercept);
    }
    return result;
}

bool float_equal(float a, float b){
    return (std::fabs(a-b) < 0.00001);
}

TEST_CASE("Swing"){
    double error_bound = 0.3;
    Swing p(error_bound, 1);
    // p = p.getSwing(error_bound);
    CHECK(p.fitValueSwing (1, 1.0) == 1);
    CHECK(p.fitValueSwing(2, 1.3) == 1);
    CHECK(p.fitValueSwing(3, 1.24) == 1);
    CHECK(p.fitValueSwing(4, 1.045) == 1);
    CHECK(p.fitValueSwing(5, 1.23) == 1);
    CHECK(p.fitValueSwing(6, 1.54) == 1);
    CHECK(p.fitValueSwing(7, 1.45) == 1);
    CHECK(p.fitValueSwing(8, 1.12) == 1);
    CHECK(p.fitValueSwing(9, 1.12) == 1);

    SUBCASE("Results"){
        CHECK(float_equal(p.get_error_bound(), 0.3f));
        CHECK(float_equal(p.get_first_timestamp(), 1.0f));
        CHECK(float_equal(p.get_upper_bound_slope(), 0.0525f));
        CHECK(float_equal(p.get_upper_bound_intercept(), 0.9475f));
        CHECK(float_equal(p.get_lower_bound_slope(), 0.048f));
        CHECK(float_equal(p.get_lower_bound_intercept(), 0.952f));
    }


    SUBCASE("Swing grid"){

    //Grid
    std::vector<float> vals{1.0, 1.3, 1.24, 1.045, 1.23, 1.54, 1.45, 1.12, 1.12};
    std::vector<long> timestamps{1,2,3,4,5,6,7,8,9};

    //Get y-coordinate for first and last point
    auto y_first = p.getModelFirst();
    auto y_last = p.getModelLast();

    CHECK(y_first == 1);
    CHECK(float_equal(y_last, 1.42f));

    auto res = p.gridSwing(y_first, y_last, 1, timestamps, timestamps.size());
    bool equal = true;
    for(int i = 0; i < vals.size(); i++){
        if(std::fabs(vals[i]-res[i]) > error_bound){
            equal = false;
        }
    }

    CHECK(equal == true);
    } 
}

TEST_CASE("Not all values fit"){
    double error_bound = 0.2;
    Swing p(error_bound, 1);
    // p = p.getSwing(0.2); //lower error bounds ensures that not all values fit
    
    CHECK(p.fitValueSwing(1, 1.0) == 1);
    CHECK(p.fitValueSwing(2, 1.3) == 1);
    CHECK(p.fitValueSwing(3, 1.24) == 1);
    CHECK(p.fitValueSwing(4, 1.045) == 0);
    CHECK(p.fitValueSwing(5, 1.23) == 1);
    CHECK(p.fitValueSwing(6, 1.54) == 1);
    CHECK(p.fitValueSwing(7, 1.45) == 1);
    CHECK(p.fitValueSwing(8, 1.12) == 0);
    CHECK(p.fitValueSwing(9, 1.12) == 0);
}


