#include "bitreader.h"
#include <cstdio>
#include <iostream>


BitReader::BitReader(std::vector<uint8_t> bytes, int byteCount) {
    this->bytes = bytes;
    this->bytesCount = byteCount;
    this->nextBit = 0;
}

uint32_t readBits(BitReader *bitReader, uint8_t numberOfBits) {
    if (numberOfBits > 32) {
        printf("ERROR: readBits cannot read more than 32 bits\n");
        printf("attempted to read %d bits\n", numberOfBits);
        exit(1);
    }
    uint64_t value = 0;
    size_t startBit = bitReader->nextBit;
    size_t endBit = startBit + (size_t) numberOfBits;
    for (size_t bit = startBit; bit < endBit; bit++) {
        uint8_t byte = bitReader->bytes[bit / 8];
        size_t shift = 7 - (bit % 8);
        uint64_t temp_bit = (byte >> shift) & 1;
        value = (value << 1) | temp_bit;
    }
    bitReader->nextBit = endBit;
    return value;
}

uint32_t readBitsSigned(BitReader *bitReader, uint8_t numberOfBits) {
    if (numberOfBits > 32) {
        printf("ERROR: readBits cannot read more than 32 bits\n");
        printf("attempted to read %d bits\n", numberOfBits);
        exit(1);
    }
    uint64_t value = 0;
    size_t startBit = bitReader->nextBit;
    size_t endBit = startBit + (size_t) numberOfBits;
    bool negativeNumber = false;
    for (size_t bit = startBit; bit < endBit; bit++) {
        uint8_t byte = bitReader->bytes[bit / 8];
        size_t shift = 7 - (bit % 8);
        uint64_t temp_bit = (byte >> shift) & 1;
        value = (value << 1) | temp_bit;

        // Handle negative numbers
        if(bit == startBit){
            if(value != 0){
                negativeNumber = true;
            }
        }
    }

    // Handle negative numbers
    if(negativeNumber){
        int numberToSubtract = 1;
        numberToSubtract <<= (numberOfBits);
        value-=numberToSubtract;
    }
    bitReader->nextBit = endBit;




    return value;
}

uint32_t readBit(BitReader *bitReader) {
    return readBits(bitReader, 1);
}

float intToFloat(uint32_t value) {
    return *(float *) (&value);
}

void appendAZeroBit(BitVecBuilder *data) {
    appendBits(data, 0, 1);
}

void appendAOneBit(BitVecBuilder *data) {
    appendBits(data, 1, 1);
}

void appendBits(BitVecBuilder *data, long bits, uint8_t numberOfBits) {

    while (numberOfBits > 0) {
        uint8_t bitsWritten;

        if (numberOfBits > data->remainingBits) {
            uint8_t shift = numberOfBits - data->remainingBits;
            data->currentByte |= (uint8_t) ((bits >> shift) &
                                            ((1 << data->remainingBits) - 1));
            bitsWritten = data->remainingBits;
        } else {
            uint8_t shift = data->remainingBits - numberOfBits;

            uint8_t mask = 0b11111111; // TODO : Also do this in the case above?
            bool isNegative = bits < 0;
            if(isNegative){
                mask >>= (8-data->remainingBits);
            }

            data->currentByte |= (uint8_t) ((bits << shift) & mask);

            bitsWritten = numberOfBits;
        }
        numberOfBits -= bitsWritten;
        data->remainingBits -= bitsWritten;

        if (data->remainingBits == 0) {
            data->bytes.push_back(data->currentByte);
            data->bytesCounter = data->bytesCounter + 1;
            data->currentByte = 0;
            data->remainingBits = 8;
        }

    }
}

uint8_t getLengthOfBinaryRepresentation(int val){
    uint8_t result;
    for(result = 0; val != 0; ++result) val  >>= 1;
    if(!result){
        return 1;
    } else {
        return result;
    }
}