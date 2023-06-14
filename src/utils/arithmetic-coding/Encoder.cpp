#include <algorithm>
#include <iostream>
#include <cmath>
#include <cassert>
#include "Encoder.h"

// Adapted from https://github.com/KyrietS/arithmetic-coding/blob/f80e7a911e898a45d114a1103149ad3b5feb0f40/AC_Core/src/AdaptiveScalingCoder.cpp

Encoder::Encoder() {
    out.currentByte = 0;
    out.remainingBits = 8;
    out.bytesCounter = 0;
}

///
/// \param uncompressed Vector of integers to be counted
/// \return Map with key = integer and value = frequency
std::map<int, int> Encoder::countFrequencies(const std::vector<int> &uncompressed){
    std::map<int, int> frequencies;
    for(const auto elem : uncompressed){
        auto it = frequencies.find(elem);
        if ( it == frequencies.end() ) {
            // not found
            frequencies[elem] = 1;
        } else {
            frequencies[elem]++;
        }
    }
    return frequencies;
}


///
/// \param in Vector to be encoded
/// \param uniqueVals (output) Map containing unique elements where key = index and value = value
/// \param accFreqs (output) Vector of float containing accumulated frequencies for the values (range from 0 to 1)
/// \return Encoding represented as a vector of bytes
std::vector<uint8_t> Encoder::encode(std::vector<int> in, std::map<int, int> *uniqueVals, std::vector<float> *accFreqs)
{
    // Count frequencies for all values
    auto freqsMap = countFrequencies(in);

    // Initialise the accumulated frequencies vector with '0'
    accFreqs->push_back(0);

    // Construct map uniqueVals that maps an index (key) to each unique value in 'in' (value)
    auto sorted = in;
    std::sort(sorted.begin(), sorted.end());
    int mapKey = 0;
    for(int i = 0; i < sorted.size(); i++){
        if(i!=0){
            if(sorted.at(i) != sorted.at(i-1)){
                (*uniqueVals)[mapKey] = sorted.at(i);
                mapKey++;
                accFreqs->push_back(i); // Store the accumulated frequencies
            }
        } else {
            (*uniqueVals)[mapKey] = sorted.at(i);
            mapKey++;
        }
    }
    // Finish accumulated frequencies vector with the full size of the input
    accFreqs->push_back(in.size());

    // Map accumulated frequencies to range 0-1
    for(int i = 0; i < accFreqs->size(); i++){
        accFreqs->at(i) /= in.size();
    }

    // Initialise interval range to full range (i.e. 0 to 2^32).
    // The output encoding represents a value within this range that represents the entire input.
    // The algorithm essentially narrows down the interval from full range to that very specific value
    uint64_t a = 0L;
    uint64_t b = WHOLE;
    int s = 0L;

    bool endOfInput = false;
    while (!endOfInput)
    {
        // Get next element to be encoded
        int byte;
        if(!in.empty()){
            byte = in.front();
            in.erase(in.begin());
        } else {
            break;
        }

        // We encode indices that represent the correct values. When decoding, we use these indices
        // to look up the correct values from the 'uniqueVals' map.
        int key = -1;
        for (auto it = (*uniqueVals).begin(); it != (*uniqueVals).end(); ++it)
            if (it->second == byte){
                key = it->first;
            }

        // We expect to find indices for all values
        assert(key != -1);

        // Calculate size of current interval. 'a' and 'b' are the bounds of the interval
        uint64_t w = b - a;

        // Calculate the sub-interval within the current interval and update 'a' and 'b'
        b = a +  llround(w * (double)accFreqs->at(key+1));
        a = a +  llround(w * (double)accFreqs->at(key));

        // The smaller the interval gets, the closer 'a' and 'b' will get to each other. To avoid
        // precision issues, we rescale the entire interval in three cases.
        // It's crucial to perform the exact same operations at the exact same times when decoding

        // Case 1: The entire sub-interval is entirely contained within one of the halfs of the current interval
        while (b < HALF || a > HALF)
        {
            // Left half
            if (b < HALF) {
                // A 0-bit means that the sub-interval is placed in the left half
                appendAZeroBit(&out);

                // Handle the case where the sub-interval is in the middle two quarters
                // See the while-loop below
                for(int i = 0; i < s; i++){
                    appendAOneBit(&out);
                }
                s = 0;
                a *= 2;
                b *= 2;

            }
            // Right half
            else if (a > HALF) {
                // A 1-bit means that the sub-interval is placed in the right half
                appendAOneBit(&out);

                // Handle the case where the sub-interval is in the middle two quarters
                // See the while-loop below
                for(int i = 0; i < s; i++){
                    appendAZeroBit(&out);
                }
                s = 0;
                a = 2 * (a - HALF);
                b = 2 * (b - HALF);
            }
        }

        // Handle the case where the sub-interval is in the middle two quarters
        // See https://www.youtube.com/watch?v=7uHOGgVZUoM&ab_channel=mathematicalmonk for details
        // (around 13:30)
        while (a > QUARTER && b < 3 * QUARTER)
        {
            s += 1;
            a = 2 * (a - QUARTER);
            b = 2 * (b - QUARTER);
        }
    }

    s += 1;

    // (See https://www.youtube.com/watch?v=i_4suhZ2yNs&ab_channel=mathematicalmonk (around 13:30) for details)
    // Here, we have two cases left:
    // Case 1: The second quarter is contained within the sub-interval
    // |--a--|-----|--b--|-----|
    if (a <= QUARTER && b >= 2*QUARTER) {
        appendAZeroBit(&out);
        for(int i = 0; i < s; i++){
            appendAOneBit(&out);
        }
    }
    // Case 2: The third quarter is contained within the sub-interval
    // |-----|--a--|-----|--b--|
    else if (a <= 2*QUARTER && b > 3*QUARTER){
        appendAOneBit(&out);
        for(int i = 0; i < s; i++){
            appendAZeroBit(&out);
        }
    }

    out.bytes.push_back(out.currentByte);
    return out.bytes;
}