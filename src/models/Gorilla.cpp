#include "Gorilla.h"
#include <cmath>
#define GORILLA_MAX 50
#ifndef NDEBUG
#include "../doctest.h"
#endif

const uint8_t SIZE_OF_32INT = (uint8_t) sizeof(int32_t) * 8;

uint8_t Gorilla::leadingZeros(const uint32_t &num) {
    // Equivalent to
    // 10000000 00000000 00000000 00000000
    int msb = 1 << (SIZE_OF_32INT - 1);

    uint8_t count = 0;

    /* Iterate over each bit */
    for (int i = 0; i < SIZE_OF_32INT; i++) {
        /* If leading set bit is found */
        if ((num << i) & msb) {
            /* Terminate the loop */
            break;
        }

        count++;
    }
    return count;
}

uint8_t Gorilla::trailingZeros(const uint32_t &num) {

    uint8_t count = 0;

    // Iterate over each bit of the number.
    for (int i = 0; i < SIZE_OF_32INT; i++) {
        // If set bit is found, terminate from loop.
        if ((num >> i) & 1) {
            break;
        }
        // Increment trailing zeros count
        count++;
    }

    return count;
}

uint32_t Gorilla::floatToBit(float val) {
    return *(uint32_t *) &val;
}

bool Gorilla::fitValueGorilla(float value) {
    if (length >= GORILLA_MAX){
        return false;
    }
    uint32_t valueAsInteger = floatToBit(
            value); // Read the binary representation of the float value as an integer, which can then be used for bitwise operations.
    uint32_t lastValueAsInteger = floatToBit(lastValue);
    uint32_t valueXorLastValue = valueAsInteger ^ lastValueAsInteger;

    if (compressedValues.bytesCounter == 0) {
        appendBits(&compressedValues, valueAsInteger, VALUE_SIZE_IN_BITS);

    } else if (valueXorLastValue == 0) {
        appendAZeroBit(&compressedValues);
    } else {
        uint8_t leadingZeroBits = leadingZeros(valueXorLastValue);
        uint8_t trailingZeroBits = trailingZeros(valueXorLastValue);
        appendAOneBit(&compressedValues);

        if (leadingZeroBits >= lastLeadingZeroBits
            && trailingZeroBits >= lastTrailingZeroBits) {
            appendAZeroBit(&compressedValues);
            uint8_t meaningfulBits = VALUE_SIZE_IN_BITS
                                     - lastLeadingZeroBits
                                     - lastTrailingZeroBits;
            appendBits(&compressedValues,
                       valueXorLastValue >> lastTrailingZeroBits,
                       meaningfulBits
            );

        } else {
            appendAOneBit(&compressedValues);
            appendBits(&compressedValues, leadingZeroBits, 5);
            uint8_t meaningfulBits =
                    VALUE_SIZE_IN_BITS - leadingZeroBits - trailingZeroBits;
            appendBits(&compressedValues, meaningfulBits, 6);
            appendBits(&compressedValues,
                       valueXorLastValue >> trailingZeroBits,
                       meaningfulBits);

            lastLeadingZeroBits = leadingZeroBits;
            lastTrailingZeroBits = trailingZeroBits;

        }
    }

    lastValue = value;
    length++;
    return true;
}

float Gorilla::getBytesPerValue() const {
    return static_cast<float>(len(compressedValues)) /
           static_cast<float>(length);
}

size_t Gorilla::len(const BitVecBuilder &data) {
    return data.bytesCounter + (size_t) (data.remainingBits != 8);
}

Gorilla::Gorilla() {
    lastValue = 0;
    lastLeadingZeroBits = UCHAR_MAX;
    lastTrailingZeroBits = 0;
    compressedValues.currentByte = 0;
    compressedValues.remainingBits = 8;
    compressedValues.bytesCounter = 0;
    length = 0;
}

std::vector<float>
Gorilla::gridGorilla(std::vector<uint8_t> values, int valuesCount,
            int timestampCount) {
        std::vector<float> result;
        BitReader bitReader(values, valuesCount);
        int leadingZeros = 255;
        int trailingZeros = 0;
    uint32_t lastValue = readBits(&bitReader, VALUE_SIZE_IN_BITS);
    result.push_back(intToFloat(lastValue));
    for (int i = 0; i < timestampCount - 1; i++) {
        if (readBit(&bitReader)) {
            if (readBit(&bitReader)) {
                leadingZeros = readBits(&bitReader, 5);
                uint8_t meaningfulBits = readBits(&bitReader, 6);
                if (meaningfulBits == 63) {
                    for (int j = 0; j < valuesCount; j++) {
                        printf("ERROR %d,", values.at(j));
                    }
                }
                trailingZeros =
                        VALUE_SIZE_IN_BITS - meaningfulBits - leadingZeros;
            }

            uint8_t meaningfulBits =
                    VALUE_SIZE_IN_BITS - leadingZeros - trailingZeros;
            uint32_t value = readBits(&bitReader, meaningfulBits);
            value <<= trailingZeros;
            value ^= lastValue;
            lastValue = value;
        }

        result.push_back(intToFloat(lastValue));
    }
    return result;
}

std::vector<uint8_t> Gorilla::getNFirstValuesBitstring(int N, std::vector<uint8_t> bytes) {

    int count = 0;
    BitVecBuilder result;
    BitReader bitReader(bytes, bytes.size());
    int leadingZeros = 255;
    int trailingZeros = 0;
    uint32_t lastValue = readBits(&bitReader, VALUE_SIZE_IN_BITS);
    appendBits(&result, lastValue, VALUE_SIZE_IN_BITS);
    for (int i = 0; i < N-1; i++) {
        if (readBit(&bitReader)) {
            appendAOneBit(&result);
            if (readBit(&bitReader)) {
                appendAOneBit(&result);

                leadingZeros = readBits(&bitReader, 5);
                appendBits(&result, leadingZeros, 5);
                uint8_t meaningfulBits = readBits(&bitReader, 6);
                appendBits(&result, meaningfulBits, 6);
                if (meaningfulBits == 63) {
                    for (int j = 0; j < bytes.size(); j++) {
                        printf("ERROR %d,", bytes.at(j));
                    }
                }
                trailingZeros =
                        VALUE_SIZE_IN_BITS - meaningfulBits - leadingZeros;
            } else {
                appendAZeroBit(&result);
            }

            uint8_t meaningfulBits =
                    VALUE_SIZE_IN_BITS - leadingZeros - trailingZeros;
            uint32_t value = readBits(&bitReader, meaningfulBits);
            appendBits(&result, value, meaningfulBits);


            value <<= trailingZeros;
            value ^= lastValue;
            lastValue = value;
        } else {
            appendAZeroBit(&result);
        }
    }

    result.bytes.push_back(result.currentByte);

    return result.bytes;
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"

#ifndef NDEBUG
TEST_CASE("GORILLA TESTS") {
    std::vector<uint8_t> original{
            63, 128, 0, 0, 212, 172, 204, 204, 238, 55, 141, 107, 87, 111, 91,
            182, 44,
            138, 244, 171, 97, 184, 125, 43, 124, 135, 35, 169, 109, 177, 108,
            192};

    std::vector<float> values{1.0, 1.3, 1.24, 1.045, 1.23, 1.54, 1.45, 1.12,
                              1.12};
    Gorilla g;

    for (auto v: values) {
        g.fitValueGorilla(v);
    }
    g.compressedValues.bytes.push_back(
            g.compressedValues.currentByte); // 192 is not pushed when fitting the values. This line does that manually.

    SUBCASE("Size equal to original") {
        CHECK(original.size() == g.compressedValues.bytes.size());
    }
    SUBCASE("Values equal to original") {
        bool equal = true;
        for (int i = 0; i < original.size(); i++) {
            if (original[i] != (int) g.compressedValues.bytes[i]) {
                equal = false;
            }
        }
        CHECK(equal == true);
    }

    SUBCASE("Gorilla grid") {
        bool equal = true;
        auto res = g.gridGorilla(original, original.size(), values.size());
        for (int i = 0; i < values.size(); i++) {
            if (std::fabs(values[i] - res[i]) >
                0.00001) { //there might be some float inaccuracy
                equal = false;
            }
        }
        CHECK(equal == true);
    }
    SUBCASE("getNFirstValuesBitstring") {
        bool equal = true;
        for(int N = 1; N < values.size(); N++){
            auto res = g.getNFirstValuesBitstring(N, original);
            auto decompressed = g.gridGorilla(res, res.size(), N);
            for (int i = 0; i < N; i++) {
                if (std::fabs(values[i] - decompressed[i]) >
                    0.00001) { //there might be some float inaccuracy
                    equal = false;
                }
            }
        }

        CHECK(equal == true);
    }

}

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"

TEST_CASE("Leading and trailing zeros") { //Taken from Rust
    Gorilla g;
    g.fitValueGorilla(37.0);
    g.fitValueGorilla(71.0);
    g.fitValueGorilla(73.0);
    CHECK(g.lastLeadingZeroBits == 8);
    CHECK(g.lastTrailingZeroBits == 17);
}
#endif
#pragma clang diagnostic pop

