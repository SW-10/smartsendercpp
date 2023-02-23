#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "../utils/bitreader.h"
#include "../constants.h"
#include <stdio.h>
#include <limits.h>
#include <vector>

struct Bit_vec_builder {
    uint8_t current_byte ;
    uint8_t remaining_bits;
    int bytes_capacity;
    int bytes_counter;
    std::vector<uint8_t> bytes;
    
} typedef Bit_vec_builder;

class Gorilla {
private:
        float last_value;
        uint8_t last_leading_zero_bits;
        uint8_t last_trailing_zero_bits;
        size_t length;

        void append_bits(Bit_vec_builder* data, long bits, uint8_t number_of_bits);
        void append_a_zero_bit(Bit_vec_builder* data);
        void append_a_one_bit(Bit_vec_builder* data);
        float get_bytes_per_value_gorilla();
        size_t get_length_gorilla();
        size_t len(const Bit_vec_builder &data);
        std::vector<uint8_t> get_compressed_values();

        uint8_t leading_zeros(const uint32_t &num);
        uint8_t trailing_zeros(const uint32_t &num);
        uint32_t float_to_bit(float val);

        void reset_gorilla();
        std::vector<uint8_t> finish(Bit_vec_builder* data);
        std::vector<uint8_t> finish_with_one_bits(Bit_vec_builder* data);

public: 
        void fitValueGorilla(float value);
        std::vector<float> grid_gorilla(std::vector<uint8_t> values, int values_count, int timestamp_count);
        Bit_vec_builder compressed_values;

        Gorilla get_gorilla();

        uint8_t get_last_leading_zero_bits() { return last_leading_zero_bits; }
        uint8_t get_last_trailing_zero_bits() { return last_trailing_zero_bits; }        

};