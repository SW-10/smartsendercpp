#ifndef PMCMEAN
#define PMCMEAN
#include <stdint.h>
#include <stdlib.h>
#include <vector>
struct Pmc_mean {
    public:
        double error;
        float min_value;
        float max_value;
        float sum_of_values;
        size_t length;

        int fit_value_pmc(float value, int is_error_absolute);
        int is_value_within_error_bound(float, float, int);
        int equal_or_nan_pmc(float, float);
        int is_nan_pmc(float);
        float get_bytes_per_value_pmc(Pmc_mean* data);
        float get_model_pmcmean(Pmc_mean* data);
        size_t get_length_pmcmean (Pmc_mean* data);

        Pmc_mean get_pmc_mean(double error_bound);
        void reset_pmc_mean(Pmc_mean *pmc);
        std::vector<float> grid_pmc_mean(float value, int timestamp_count);
} typedef Pmc_mean;


#endif