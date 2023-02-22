#include "gorilla.h"
#include <cmath>
#include "../doctest.h"

const int debug = 0;

const uint8_t SIZE_OF_32INT = (uint8_t) sizeof(int32_t) * 8;

uint8_t Gorilla::leading_zeros(const uint32_t &num){
    // Equivalent to
    // 10000000 00000000 00000000 00000000
    int msb = 1 << (SIZE_OF_32INT - 1);

    uint8_t count = 0;

    /* Iterate over each bit */
    for(int i=0; i<SIZE_OF_32INT; i++)
    {
        /* If leading set bit is found */
        if((num << i) & msb)
        {
            /* Terminate the loop */
            break;
        }

        count++;
    }

    if(debug) printf("Total number of leading zeros in %d is %d", num, count);
    return count;
}
uint8_t Gorilla::trailing_zeros(const uint32_t &num){
    
    uint8_t count = 0;

    /* Iterate over each bit of the number */
    for(int i=0; i< SIZE_OF_32INT;  i++)
    {
        /* If set bit is found the terminate from loop*/
        if((num >> i ) & 1)
        {
            /* Terminate from loop */
            break;
        }

        /* Increment trailing zeros count */
        count++;
    }

    if(debug) printf("Total number of trailing zeros in %d is %d.", num, count);
    return count;
}

uint32_t Gorilla::float_to_bit(float val){
    return *(uint32_t *)&val;
}


void Gorilla::fitValueGorilla(float value){
    uint32_t value_as_integer = float_to_bit(value); // Læs den binære repræsentation af float value som en integer, som vi herefter kan lave bitwise operationer på
    uint32_t last_value_as_integer = float_to_bit(last_value);
    uint32_t value_xor_last_value = value_as_integer ^ last_value_as_integer;
    
    if(compressed_values.bytes_counter == 0){
        append_bits(&compressed_values, value_as_integer, VALUE_SIZE_IN_BITS);

    } else if (value_xor_last_value == 0){
        append_a_zero_bit(&compressed_values);
    } else {
        uint8_t leading_zero_bits = leading_zeros(value_xor_last_value);
        uint8_t trailing_zero_bits = trailing_zeros(value_xor_last_value);
        append_a_one_bit(&compressed_values);

        if(leading_zero_bits >= last_leading_zero_bits
            && trailing_zero_bits >= last_trailing_zero_bits)
        {
            append_a_zero_bit(&compressed_values);
            uint8_t meaningful_bits = VALUE_SIZE_IN_BITS 
                - last_leading_zero_bits 
                - last_trailing_zero_bits;
            append_bits(&compressed_values, 
                value_xor_last_value >> last_trailing_zero_bits, 
                meaningful_bits
            );

        } else {
            append_a_one_bit(&compressed_values);
            append_bits(&compressed_values, leading_zero_bits, 5);
            uint8_t meaningful_bits = VALUE_SIZE_IN_BITS - leading_zero_bits - trailing_zero_bits;
            append_bits(&compressed_values, meaningful_bits, 6);
            append_bits(&compressed_values, value_xor_last_value >> trailing_zero_bits, meaningful_bits);

            last_leading_zero_bits = leading_zero_bits;
            last_trailing_zero_bits = trailing_zero_bits;

        }
    }
    
    last_value = value;
    length++;
}

float Gorilla::get_bytes_per_value_gorilla(){
    return (float) len(compressed_values) / (float) length;

}

size_t Gorilla::len(const Bit_vec_builder &data){
    return data.bytes_counter + (size_t) (data.remaining_bits != 8);
}

/* // FUNCTIONS SAVED FOR LATER USAGE
std::vector<uint8_t> Gorilla::finish(Bit_vec_builder* data){
  if (data->remaining_bits != 8){
    data->bytes_capacity++;
    data->bytes[data->bytes_counter++] = data->current_byte;
  }
  return data->bytes;
}

std::vector<uint8_t> Gorilla::finish_with_one_bits(Bit_vec_builder* data){
    if(data->remaining_bits != 8){
        uint8_t remaining_bits = pow(2, data->remaining_bits)-1;
        append_bits(data, remaining_bits, data->remaining_bits);
    }
    return finish(data);
}

std::vector<uint8_t> Gorilla::get_compressed_values(){
  return finish(&compressed_values);
}

size_t Gorilla::get_length_gorilla(){
    return length;
}

*/
void Gorilla::append_a_zero_bit(Bit_vec_builder* data){
    append_bits(data, 0, 1);
}

void Gorilla::append_a_one_bit(Bit_vec_builder* data){
    append_bits(data, 1, 1);
}


void Gorilla::append_bits(Bit_vec_builder* data, long bits, uint8_t number_of_bits){
    uint8_t _number_of_bits = number_of_bits;

    while(_number_of_bits > 0){
        uint8_t bits_written;

        if(_number_of_bits > data->remaining_bits){
            uint8_t shift = _number_of_bits -  data->remaining_bits;
            data -> current_byte |= (uint8_t)((bits >> shift) & ((1 << data->remaining_bits) - 1));
            bits_written = data->remaining_bits;
        } else {
            uint8_t shift = data->remaining_bits - _number_of_bits;
            data -> current_byte |= (uint8_t)(bits << shift);
            bits_written = _number_of_bits;
        }
        _number_of_bits -= bits_written;
        data->remaining_bits -= bits_written;

        if(data->remaining_bits == 0){
            data->bytes.push_back(data->current_byte);
            data->bytes_counter = data->bytes_counter+1;
            data->current_byte = 0;
            data->remaining_bits = 8;   
        }

    }
}

Gorilla::Gorilla(){
    last_value = 0;
    last_leading_zero_bits = UCHAR_MAX;
    last_trailing_zero_bits = 0;
    compressed_values.current_byte = 0;
    compressed_values.remaining_bits = 8;
    compressed_values.bytes_counter = 0;
    length = 0;
}


std::vector<float> Gorilla::grid_gorilla(std::vector<uint8_t> values, int values_count, int timestamp_count){
    std::vector<float> result;
    BitReader bitReader = tryNewBitreader(values, values_count);
    int leadingZeros = 255;
    int trailingZeros = 0;
    uint32_t lastValue = read_bits(&bitReader, VALUE_SIZE_IN_BITS);
    result.push_back(intToFloat(lastValue));
    for(int i = 0; i < timestamp_count-1; i++){
        if(read_bit(&bitReader)){
            if(read_bit(&bitReader)){
                leadingZeros = read_bits(&bitReader, 5);
                uint8_t meaningfulBits = read_bits(&bitReader, 6);
                if(meaningfulBits == 63){
                    for(int j = 0; j < values_count; j++){
                        printf("ERROR %d,", values[j]);
                    }
                }
                trailingZeros = VALUE_SIZE_IN_BITS - meaningfulBits - leadingZeros;
            }

            uint8_t meaningfulBits = VALUE_SIZE_IN_BITS - leadingZeros - trailingZeros;
            uint32_t value = read_bits(&bitReader, meaningfulBits);
            value <<= trailingZeros;
            value ^= lastValue;
            lastValue = value;
        }

        result.push_back(intToFloat(lastValue));
    }
    return result;
}


TEST_CASE("GORILLA TESTS") {
    std::vector<uint8_t> original{ 
        63, 128, 0, 0, 212, 172, 204, 204, 238, 55, 141, 107, 87, 111, 91, 182, 44, 
        138, 244, 171, 97, 184, 125, 43, 124, 135, 35, 169, 109, 177, 108, 192};

    std::vector<float> values{1.0, 1.3, 1.24, 1.045, 1.23, 1.54, 1.45, 1.12, 1.12};
    Gorilla g;
    
    for(auto v : values){
        g.fitValueGorilla(v);
    }
    g.compressed_values.bytes.push_back(g.compressed_values.current_byte); // 192 is not pushed when fitting the values. This line does that manually 
    
    SUBCASE("Size equal to original") {
        CHECK(original.size() == g.compressed_values.bytes.size()); 
    }
    SUBCASE("Values equal to original"){
        bool equal = true;
        for(int i = 0; i < original.size(); i++){
            if(original[i] != (int)g.compressed_values.bytes[i]){
                equal = false;
            }
        }
        CHECK(equal == true);
    }

    SUBCASE("Gorilla grid"){
        bool equal = true;
        auto res = g.grid_gorilla(original, original.size(), values.size());
        for(int i = 0; i < values.size(); i++){
            if(std::fabs(values[i] - res[i]) > 0.00001){ //there might be some float inaccuracy
                equal = false;
            }
        }
        CHECK(equal == true);
    }
}

TEST_CASE("Leading and trailing zeros"){ //Taken from Rust
    Gorilla g;
    g.fitValueGorilla(37.0);
    g.fitValueGorilla(71.0);
    g.fitValueGorilla(73.0);
    CHECK(g.get_last_leading_zero_bits() == 8);
    CHECK(g.get_last_trailing_zero_bits() == 17);
}

