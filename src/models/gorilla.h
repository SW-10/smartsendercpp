#ifndef GORILLA
#define GORILLA

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
    // uint8_t* bytes;
    std::vector<uint8_t> bytes;
    
} typedef Bit_vec_builder;

struct Gorilla {
    public:
        float last_value;
        uint8_t last_leading_zero_bits;
        uint8_t last_trailing_zero_bits;
        Bit_vec_builder compressed_values;
        size_t length;

        void append_bits(Bit_vec_builder* data, long bits, uint8_t number_of_bits);
        void append_a_zero_bit(Bit_vec_builder* data);
        void append_a_one_bit(Bit_vec_builder* data);
        void fitValueGorilla(float value);
        float get_bytes_per_value_gorilla();
        size_t get_length_gorilla();
        size_t len(const Bit_vec_builder &data);
        std::vector<uint8_t> get_compressed_values();

        uint8_t leading_zeros(const uint32_t &num);
        uint8_t trailing_zeros(const uint32_t &num);
        uint32_t float_to_bit(float val);

        Gorilla get_gorilla(void);
        void reset_gorilla();
        std::vector<float> grid_gorilla(uint8_t* values, int values_count, int timestamp_count);
        std::vector<uint8_t> finish(Bit_vec_builder* data);
        std::vector<uint8_t> finish_with_one_bits(Bit_vec_builder* data);
} typedef Gorilla;


#endif