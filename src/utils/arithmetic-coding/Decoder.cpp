#include <cmath>
#include "Decoder.h"
#include "../bitreader.h"

// Adapted from https://github.com/KyrietS/arithmetic-coding/blob/f80e7a911e898a45d114a1103149ad3b5feb0f40/AC_Core/src/AdaptiveScalingCoder.cpp

///
/// \param bytes Vector of int containing the encoded bit string
/// \param uniqueVals Map containing unique elements where key = index and value = value
/// \param accFreqs Vector of float containing accumulated frequencies for the values (range from 0 to 1)
/// \return Vector of decompressed integers
std::vector<int>  Decoder::decode(std::vector<uint8_t> bytes, std::map<int, int> uniqueVals, std::vector<float> accFreqs)
{
    // Number of unique symbols is equal to the number of elements in the vector returned from the encoder
    TOTAL_NUMBER_OF_SYMBOLS = uniqueVals.size();

    BitReader in(bytes, bytes.size());

    // Initialise interval values
    // Also initialise z (an integer representation of the encoded bit-string)
    uint64_t a = 0L;
    uint64_t b = WHOLE;
    uint64_t z = 0L;

    size_t i = 1;

    // Load the encoded bit-string into z
    while (i <= PRECISION) {
        if (readBit(&in)) {
            z += powerOf(2, PRECISION - i);
        }
        i += 1;
    }

    // Decode z!
    while (true)
    {
        // Loop through all unique symbol indices
        for (int symbol = 0; symbol < TOTAL_NUMBER_OF_SYMBOLS; symbol++)
        {
            // Calculate size of current interval. 'a' and 'b' are the bounds of the interval
            uint64_t w = b - a;

            // Calculate the sub-interval within the current interval and update 'a' and 'b'
            uint64_t b0 = a + llround(w * (double)accFreqs.at(symbol+1));
            uint64_t a0 = a + llround(w * (double)accFreqs.at(symbol));

            // If z falls within the current sub-interval, we have found the correct index
            if (a0 <= z && z < b0) {

                // We look the correct value using the found integer
                int foundInt = uniqueVals[symbol];

                // Return if we find -3 (the stop code)
                if (foundInt == -3) {
                    return result;
                }

                // Store found value
                result.push_back(foundInt);

                // Update interval values
                a = a0;
                b = b0;

                break;
            }
        }
        // Scaling follows same logic as in the encoder (see those comments).
        // Here, however, we also need to scale z in the same manner as the interval bounds
        while (b < HALF || a > HALF)
        {
            if (b < HALF) {
                a *= 2;
                b *= 2;
                z *= 2;
            }
            else if (a > HALF) {
                a = 2 * (a - HALF);
                b = 2 * (b - HALF);
                z = 2 * (z - HALF);
            }

            // Update z
            if (readBit(&in))
                z += 1;
            i += 1;
        }

        // Scaling "blow up"
        while (a > QUARTER && b < 3 * QUARTER)
        {

            a = 2 * (a - QUARTER);
            b = 2 * (b - QUARTER);
            z = 2 * (z - QUARTER);

            // Update z
            if (readBit(&in))
                z += 1;
            i += 1;
        }
    }
}