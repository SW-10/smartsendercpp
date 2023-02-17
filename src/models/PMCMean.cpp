#include "PMCMean.h"
#include "../constants.h"
#include <cmath>
#include <cstdio>
#include "../doctest.h"

int Pmc_mean::fit_value_pmc(float value, int is_error_absolute){
    float next_min_value = min_value < value ? min_value : value;
    float next_max_value = max_value > value ? max_value : value;
    float next_sum_of_values = sum_of_values + value;
    size_t next_length = length+1;
    float average = (next_sum_of_values / next_length);

    if(is_value_within_error_bound(next_min_value, average, is_error_absolute) && is_value_within_error_bound(next_max_value, average, is_error_absolute)){
        min_value = next_min_value;
        max_value = next_max_value;
        sum_of_values = next_sum_of_values;
        length = next_length;
        return 1;
    } else {
        return 0;
    }
}

int Pmc_mean::is_value_within_error_bound(float real_value, float approx_value, int is_error_absolute){
    if(equal_or_nan_pmc(real_value, approx_value)){
        return 1;
    } else {
        float difference = real_value - approx_value;
        float result = fabsf(difference / real_value);
        if(is_error_absolute){  // check if relative or absolute error
            return fabsf(difference) <= error;
        }
        return (result * 100) <= error;
    }
}

float Pmc_mean::get_bytes_per_value_pmc(Pmc_mean* data){
    return (float) VALUE_SIZE_IN_BYTES / (float) data -> length;
}


int Pmc_mean::equal_or_nan_pmc(float v1, float v2){
    return v1==v2 || (std::isnan(v1) && std::isnan(v2));
}


float Pmc_mean::get_model_pmcmean(Pmc_mean* data){
    return (float) (data->sum_of_values / (double) data->length);
}

size_t Pmc_mean::get_length_pmcmean (Pmc_mean* data){
    return data->length;
}

Pmc_mean Pmc_mean::get_pmc_mean(double error_bound){
  Pmc_mean data;
  data.error = error_bound;
  data.min_value = NAN;
  data.max_value = NAN;
  data.sum_of_values = 0;
  data.length = 0;
  return data;
}

void Pmc_mean::reset_pmc_mean(Pmc_mean *pmc){
  pmc->min_value = NAN;
  pmc->max_value = NAN;
  pmc->sum_of_values = 0;
  pmc->length = 0;
}

std::vector<float> Pmc_mean::grid_pmc_mean(float value, int timestamp_count){
    std::vector<float> result;
    for(int i = 0; i < timestamp_count; i++){
        result.push_back(value);
    }

    return result;
}

TEST_CASE("All values fit"){
    Pmc_mean p;
    p = p.get_pmc_mean(0.5);
    p.fit_value_pmc(1.0 , 1);
    p.fit_value_pmc(1.3 , 1);
    p.fit_value_pmc(1.24, 1);
    p.fit_value_pmc(1.045, 1);

    CHECK(p.get_error() == 0.5f);
    CHECK(p.get_length() == 4);
    CHECK(p.get_max_value() == 1.3f);
    CHECK(p.get_min_value() == 1.0f);
    CHECK(p.get_sum_of_values() == 4.585f);

    p.fit_value_pmc(0.9, 1);
    p.fit_value_pmc(1.54, 1);
    p.fit_value_pmc(1.45, 1);
    p.fit_value_pmc(1.12, 1);
    p.fit_value_pmc(1.12, 1);
    
    CHECK(p.get_error() == 0.5f);
    CHECK(p.get_length() == 9);
    CHECK(p.get_max_value() == 1.54f);
    CHECK(p.get_min_value() == 0.9f);
    CHECK(p.get_sum_of_values() == 10.715f);
}

TEST_CASE("Not all values fit"){
    Pmc_mean p;
    p = p.get_pmc_mean(0.2); //lower error bounds ensures that not all values fit
    
    CHECK(p.fit_value_pmc(1.0 , 1)  == 1);
    CHECK(p.fit_value_pmc(1.3 , 1)  == 1);
    CHECK(p.fit_value_pmc(1.24, 1)  == 1);
    CHECK(p.fit_value_pmc(1.045, 1) == 1);
    CHECK(p.fit_value_pmc(0.9, 1)   == 0);
    CHECK(p.fit_value_pmc(1.54, 1)  == 0);
    CHECK(p.fit_value_pmc(1.45, 1)  == 0);
    CHECK(p.fit_value_pmc(1.12, 1)  == 1);
    CHECK(p.fit_value_pmc(1.12, 1)  == 1);
}

TEST_CASE("Grid"){

    //Grid
    Pmc_mean p;
    float error_bound = 1;
    p = p.get_pmc_mean(error_bound);
    std::vector vals{1.0, 1.3, 1.24, 1.045, 0.9, 1.54, 1.45, 1.12, 1.12};
    for(auto v : vals){
        p.fit_value_pmc(v, 1);
    }

    auto res = p.grid_pmc_mean(p.get_sum_of_values()/p.get_length(), vals.size());
    bool equal = true;
    for(int i = 0; i < vals.size(); i++){
        if(std::fabs(vals[i]-res[i]) > error_bound){
            equal = false;
        }
    }

    CHECK(equal == true);
}

