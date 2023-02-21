#include "PMCMean.h"
#include "../constants.h"
#include <cmath>
#include <cstdio>
#include "../doctest.h"

#include <iostream>

Pmc_mean::Pmc_mean(double &error_bound, bool error_absolute) : error(error_bound) {
    std::cout<<"Constructor!"<<std::endl;
    min_value = NAN;
    max_value = NAN;
    sum_of_values = 0;
    length = 0;
    is_error_absolute = error_absolute;
}

int Pmc_mean::fit_value_pmc(float value){
    float next_min_value = min_value < value ? min_value : value;
    float next_max_value = max_value > value ? max_value : value;
    float next_sum_of_values = sum_of_values + value;
    size_t next_length = length+1;
    float average = (next_sum_of_values / next_length);

    if(is_value_within_error_bound(next_min_value, average) && is_value_within_error_bound(next_max_value, average)){
        min_value = next_min_value;
        max_value = next_max_value;
        sum_of_values = next_sum_of_values;
        length = next_length;
        return 1;
    } else {
        return 0;
    }
}

int Pmc_mean::is_value_within_error_bound(float real_value, float approx_value){
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

float Pmc_mean::get_bytes_per_value_pmc(){
    return (float) VALUE_SIZE_IN_BYTES / (float) length;
}


int Pmc_mean::equal_or_nan_pmc(float v1, float v2){
    return v1==v2 || (std::isnan(v1) && std::isnan(v2));
}


float Pmc_mean::get_model_pmcmean(){
    return (float) (sum_of_values / (double) length);
}

size_t Pmc_mean::get_length_pmcmean (){
    return length;
}

void Pmc_mean::reset_pmc_mean(){
  min_value = NAN;
  max_value = NAN;
  sum_of_values = 0;
  length = 0;
}

std::vector<float> Pmc_mean::grid_pmc_mean(float value, int timestamp_count){
    std::vector<float> result;
    for(int i = 0; i < timestamp_count; i++){
        result.push_back(value);
    }

    return result;
}

TEST_CASE("All values fit"){
    double error = 0.5;
    Pmc_mean p(error, true);
    p.fit_value_pmc(1.0);
    p.fit_value_pmc(1.3);
    p.fit_value_pmc(1.24);
    p.fit_value_pmc(1.045);

    CHECK(p.get_error() == 0.5);
    CHECK(p.get_length() == 4);
    CHECK(p.get_max_value() == 1.3f);
    CHECK(p.get_min_value() == 1.0f);
    CHECK(p.get_sum_of_values() == 4.585f);

    p.fit_value_pmc(0.9);
    p.fit_value_pmc(1.54);
    p.fit_value_pmc(1.45);
    p.fit_value_pmc(1.12);
    p.fit_value_pmc(1.12);
    
    CHECK(p.get_error() == 0.5);
    CHECK(p.get_length() == 9);
    CHECK(p.get_max_value() == 1.54f);
    CHECK(p.get_min_value() == 0.9f);
    CHECK(p.get_sum_of_values() == 10.715f);
}

TEST_CASE("Not all values fit"){
    double error = 0.2;
    Pmc_mean p(error, true);
    
    CHECK(p.fit_value_pmc(1.0)  == 1);
    CHECK(p.fit_value_pmc(1.3)  == 1);
    CHECK(p.fit_value_pmc(1.24)  == 1);
    CHECK(p.fit_value_pmc(1.045) == 1);
    CHECK(p.fit_value_pmc(0.9)   == 0);
    CHECK(p.fit_value_pmc(1.54)  == 0);
    CHECK(p.fit_value_pmc(1.45)  == 0);
    CHECK(p.fit_value_pmc(1.12)  == 1);
    CHECK(p.fit_value_pmc(1.12)  == 1);
}

TEST_CASE("Grid"){

    //Grid
    double error_bound = 1;
    Pmc_mean p(error_bound, true);

    std::vector vals{1.0, 1.3, 1.24, 1.045, 0.9, 1.54, 1.45, 1.12, 1.12};
    for(auto v : vals){
        p.fit_value_pmc(v);
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

