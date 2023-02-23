#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>

class PmcMean {
private:
    double error;
    float min_value;
    float max_value;
    float sum_of_values;
    size_t length;
    bool is_error_absolute;

    int is_value_within_error_bound(float, float, bool);
    int equal_or_nan_pmc(float, float);
    float get_bytes_per_value_pmc();
    float get_model_pmcmean();
    size_t get_length_pmcmean ();

    void reset_pmc_mean();

public:
    PmcMean(double error_bound, bool is_error_absolute);
    int fit_value_pmc(float value);
    std::vector<float> grid_pmc_mean(float value, int timestamp_count);
    double get_error() { return error; }
    float get_min_value() { return min_value; }
    float get_max_value() { return max_value; }
    float get_sum_of_values() { return sum_of_values; }
    size_t get_length() { return length; }
    
};