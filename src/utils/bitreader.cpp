#include "bitreader.h"
#include <stdio.h>

BitReader tryNewBitreader(uint8_t* bytes, int byteCount){
    BitReader bitReader;
    bitReader.bytes = bytes;
    bitReader.bytesCount = byteCount;
    bitReader.nextBit = 0;
    return bitReader;
}

int is_empty(BitReader* bitReader){
    return (bitReader->nextBit / 8) == bitReader->bytesCount;
}

int remaining_bits(BitReader* bitReader){
    return 8 * bitReader->bytesCount - bitReader->nextBit;
}

uint32_t read_bits(BitReader* bitReader, uint8_t numberOfBits){
    if(numberOfBits > 32){
        printf("ERROR: read_bits cannot read more than 32 bits\n");
        printf("attempted to read %d bits\n", numberOfBits);
        exit(1);
    }
    uint64_t value = 0;
    size_t startBit = bitReader->nextBit;
    size_t endBit = startBit + (size_t)numberOfBits;
    for(size_t bit = startBit; bit < endBit; bit++){
        uint8_t byte = bitReader->bytes[bit/8];
        size_t shift = 7 - (bit % 8);
        uint64_t temp_bit = (byte >> shift) & 1;
        value = (value << 1) | temp_bit;
    }
    bitReader->nextBit = endBit;
    return value;
}

uint32_t read_bit(BitReader* bitReader){
    return read_bits(bitReader, 1);
}

float intToFloat(uint32_t value){
    return *(float*) (&value);
}