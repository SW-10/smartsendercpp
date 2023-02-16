#include "PMCMean.h"
#include "constants.h"
#include <math.h>
#include <stdio.h>


int fit_value_pmc(Pmc_mean *data, float value, int is_error_absolute);
int is_value_within_error_bound(Pmc_mean*, float, float, int);
int equal_or_nan_pmc(float, float);
int is_nan_pmc(float);


int fit_value_pmc(Pmc_mean* data, float value, int is_error_absolute){
    float next_min_value = data->min_value < value ? data->min_value : value;
    float next_max_value = data->max_value > value ? data->max_value : value;
    float next_sum_of_values = data->sum_of_values + value;
    size_t next_length = data->length+1;
    float average = (next_sum_of_values / next_length);

    if(is_value_within_error_bound(data, next_min_value, average, is_error_absolute) && is_value_within_error_bound(data, next_max_value, average, is_error_absolute)){
        data->min_value = next_min_value;
        data->max_value = next_max_value;
        data->sum_of_values = next_sum_of_values;
        data->length = next_length;
        return 1;
    } else {
        return 0;
    }
}

int is_value_within_error_bound(Pmc_mean* data, float real_value, float approx_value, int is_error_absolute){
    if(equal_or_nan_pmc(real_value, approx_value)){
        return 1;
    } else {
        float difference = real_value - approx_value;
        float result = fabsf(difference / real_value);
        if(is_error_absolute){  // check if relative or absolute error
            return fabsf(difference) <= data->error;
        }
        return (result * 100) <= data->error;
    }
}

float get_bytes_per_value_pmc(Pmc_mean* data){
    return (float) VALUE_SIZE_IN_BYTES / (float) data -> length;
}


int equal_or_nan_pmc(float v1, float v2){
    return v1==v2 || (is_nan_pmc(v1) && is_nan_pmc(v2));
}

int is_nan_pmc(float val){
    return val != val; //Wacky code but should work for now. Val is NAN if val != val returns 1
}

float get_model_pmcmean(Pmc_mean* data){
    return (float) (data->sum_of_values / (double) data->length);
}

size_t get_length_pmcmean (Pmc_mean* data){
    return data->length;
}

Pmc_mean get_pmc_mean(double error_bound){
  Pmc_mean data;
  data.error = error_bound;
  data.min_value = NAN;
  data.max_value = NAN;
  data.sum_of_values = 0;
  data.length = 0;
  return data;
}

void reset_pmc_mean(Pmc_mean *pmc){
  pmc->min_value = NAN;
  pmc->max_value = NAN;
  pmc->sum_of_values = 0;
  pmc->length = 0;
}

float* grid_pmc_mean(float value, int timestamp_count){
    float* result;
    result = calloc(timestamp_count, sizeof(*result));
    if(!result){
        printf("CALLOC ERROR (grid_pmc_mean: result)\n");
    }
    for(int i = 0; i < timestamp_count; i++){
        result[i] = value;
    }
    return result;
}