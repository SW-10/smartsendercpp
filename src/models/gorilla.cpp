#include "gorilla.h"
#include <math.h>
#include <iostream>
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
        // TODO: &(data->compressed_values) ?????????????????
        //printf("====================\n");
        append_bits(&compressed_values, value_as_integer, VALUE_SIZE_IN_BITS);

    } else if (value_xor_last_value == 0){
        append_a_zero_bit(&compressed_values);
        // printf("ZERO BIT\n");
    } else {
        uint8_t leading_zero_bits = leading_zeros(value_xor_last_value);
        uint8_t trailing_zero_bits = trailing_zeros(value_xor_last_value);
        append_a_one_bit(&compressed_values); //???????????????????

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
        // uint8_t leading_zero_bits = value_xor_last_value

    }
    
    last_value = value;
    length++;
}

float Gorilla::get_bytes_per_value_gorilla(){
    // return (float) len(&data->compressed_values) / (float) data->length;
    return (float) len(compressed_values) / (float) length;

}

size_t Gorilla::len(const Bit_vec_builder &data){
    return data.bytes_counter + (size_t) (data.remaining_bits != 8);
}

std::vector<uint8_t> Gorilla::finish(Bit_vec_builder* data){
  if (data->remaining_bits != 8){
    data->bytes_capacity++;
    // data->bytes = realloc(data->bytes, 4 * data->bytes_capacity * sizeof(*data->bytes));
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

            //is this correct? probs
            //printf("%d\n", 4 * data->bytes_capacity * sizeof(uint8_t));
            data->bytes_capacity++;

            // OLD C STUFF
            // data->bytes = realloc(data->bytes, 4 * data->bytes_capacity * sizeof(*data->bytes));
            // if(data->bytes == NULL){
            //     printf("REALLOC ERROR(append_bits)\n");
            // }
            data->bytes.push_back(data->current_byte);
            // data->bytes[data->bytes_counter] = data->current_byte;
            data->bytes_counter = data->bytes_counter+1;
            data->current_byte = 0;
            data->remaining_bits = 8;   
        }
    }
}

Gorilla Gorilla::get_gorilla(){
    Gorilla data;
    data.last_value = 0;
    data.last_leading_zero_bits = UCHAR_MAX;
    data.last_trailing_zero_bits = 0;

    data.compressed_values.current_byte = 0;
    data.compressed_values.remaining_bits = 8;

    //Initialise bytes array to NULL values
    data.compressed_values.bytes_counter = 0;
    
    //is this correct?
    data.compressed_values.bytes_capacity = 1;
    // OLD C STUFF
    // data.compressed_values.bytes = (uint8_t*) calloc (data.compressed_values.bytes_capacity, sizeof(uint8_t));
    // if(data.compressed_values.bytes == NULL){
    //     printf("MALLOC ERROR\n");
    // }


    // for(int i = 0; i < sizeof(data.compressed_values.bytes)/sizeof(data.compressed_values.bytes[0]); i++){
    //     data.compressed_values.bytes[i] = NULL;
    // }
    data.length = 0;
    
    return data;
}

void Gorilla::reset_gorilla(){
    last_leading_zero_bits = UCHAR_MAX;
    last_trailing_zero_bits = 0;
    last_value = 0;
    length = 0;

    compressed_values.current_byte = 0;
    compressed_values.remaining_bits = 8;
    compressed_values.bytes_capacity = 1;
    compressed_values.bytes_counter = 0;
    
    //OLD C STUFF
    // gorilla->compressed_values.bytes = realloc(gorilla->compressed_values.bytes, 4 * gorilla->compressed_values.bytes_capacity * sizeof(*gorilla->compressed_values.bytes));
    // if(gorilla->compressed_values.bytes == NULL){
    //     printf("REALLOC ERROR\n");
    // }
}

std::vector<float> Gorilla::grid_gorilla(uint8_t* values, int values_count, int timestamp_count){
    // float* result;
    // result = calloc(timestamp_count, sizeof(*result));
    // if(!result){
    //     printf("CALLOC ERROR (grid_gorilla: result)\n");
    // }

    std::vector<float> result;
    int resultIndex = 0;
    BitReader bitReader = tryNewBitreader(values, values_count);
    int leadingZeros = 255;
    int trailingZeros = 0;
    uint32_t lastValue = read_bits(&bitReader, VALUE_SIZE_IN_BITS);
    result[resultIndex++] = intToFloat(lastValue);
    for(int i = 0; i < timestamp_count-1; i++){
        if(read_bit(&bitReader)){
            if(read_bit(&bitReader)){
                leadingZeros = read_bits(&bitReader, 5);
                uint8_t meaningfulBits = read_bits(&bitReader, 6);
                if(meaningfulBits == 63){
                    for(int i = 0; i < values_count; i++){
                        printf("ERROR %d,", values[i]);
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
        result[resultIndex++] = intToFloat(lastValue);
    }
    return result;
}


TEST_CASE("GORILLA TESTS") {
    std::vector<int> original{ 
        63, 128, 0, 0, 212, 172, 204, 204, 238, 55, 141, 107, 87, 111, 91, 182, 44, 
        138, 244, 171, 97, 184, 125, 43, 124, 135, 35, 169, 109, 177, 108};

    std::vector<float> values{1.0, 1.3, 1.24, 1.045, 1.23, 1.54, 1.45, 1.12, 1.12};
   
    Gorilla g;
    g = g.get_gorilla();
    
    for(auto v : values){
        g.fitValueGorilla(v);
    }
    
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
}