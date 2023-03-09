#pragma once

#include <vector>

struct Position {
    double latitude;
    double longitude;
};

struct Vector {
    double x;
    double y;
};

class VectorBased {
public:
    VectorBased();

    int
    fitValue(long timeStamp, double latitude, double longitude, float error);

private:
    Position prev;
    Position current;
    Position start;

    long int startTime;
    long int endTime;
    unsigned int length;
    unsigned int modelLength;
    Vector vec;
    float errorSum;

    int currentDelta;
    std::vector<long> timestamps;
    int maxTimestamps;
    int currentTimestampIndex;

    double
    haversineDistance(double lat1, double lon1, double lat2, double lon2);
};