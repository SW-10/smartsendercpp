#ifndef PMCMEAN
#define PMCMEAN
#include <cstdint>
#include <cstdlib>
#include <vector>
class Pmc_mean {
private:
    double error;
    float min_value;
    float max_value;
    float sum_of_values;
    size_t length;

    int is_value_within_error_bound(float, float, int);
    int equal_or_nan_pmc(float, float);
    float get_bytes_per_value_pmc();
    float get_model_pmcmean();
    size_t get_length_pmcmean ();

    void reset_pmc_mean();

public:
    Pmc_mean get_pmc_mean(double error_bound);
    int fit_value_pmc(float value, int is_error_absolute);
    std::vector<float> grid_pmc_mean(float value, int timestamp_count);
    double get_error() { return error; }
    float get_min_value() { return min_value; }
    float get_max_value() { return max_value; }
    float get_sum_of_values() { return sum_of_values; }
    size_t get_length() { return length; }
    
} typedef Pmc_mean;


#endif