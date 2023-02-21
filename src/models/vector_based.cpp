#include "vector_based.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../doctest.h"

#define ERROR 50
#define R 6371
#define TO_RAD (3.1415926536 / 180)

VectorBased::VectorBased(){
    prev = (Position){ /*latitude =*/ 0, /*longitude =*/ 0};
    current = (Position){ /*latitude =*/ 0, /*latitude =*/ 0};
    length = 0;
    currentDelta = 0;
    modelLength = 0;
    maxTimestamps = 256;
    currentTimestampIndex = 0;

}

int VectorBased::fitValue(long timeStamp, double latitude, double longitude, float errorBound){
    if(length == 0){
        start = { latitude, longitude };
        prev = { latitude, longitude };
        startTime = timeStamp;
        length++;
        modelLength++;
        timestamps.push_back(timeStamp);
        errorSum = 0;
        return 1;
    }
    else if (length == 1) {
        currentDelta = timeStamp - startTime;

        current = { latitude, longitude };
        endTime = timeStamp;
        length++;        
        
        // Build vector
        vec.x = current.longitude - prev.longitude;
        vec.y = current.latitude - prev.latitude;

        // Scale down vector to only reflect the change for a single time step
        vec.y = vec.y / currentDelta;
        vec.x = vec.x / currentDelta;

        timestamps.push_back(timeStamp);
        modelLength++;
        return 1;
    } 
    else {
        currentDelta = timeStamp - endTime;

        // Make prediction
        // Scale up vector to reflect the change for the current delta
        Position prediction; 
        prediction.latitude = current.latitude + (vec.y * currentDelta);
        prediction.longitude = current.longitude + (vec.x * currentDelta);

        // Update current
        current = { latitude, longitude };

        // calls Distance calculator, to get distance between two points on a sphere, takes two pairs of lat,long
        // handled as a sphere, which means we get some measure of error the longer a distance is.
        double haversineDist = haversineDistance(prediction.latitude, prediction.longitude,current.latitude, current.longitude);

        //printf("Distance: %f\n", distance);
        current = prediction;

        length++;


        if(haversineDist > errorBound){
            return 0;
        } 

        //printf("Vector --- x: %f, y: %f\n", data->vec.x, data->vec.y);
        // printf("%lf, %lf\n", prediction.latitude, prediction.longitude);
        modelLength++;
        endTime = timeStamp;
        timestamps.push_back(timeStamp);

        return 1;
        
    }
}

double VectorBased::haversineDistance(double lat1, double lon1, double lat2, double lon2)
{
    double dx, dy, dz;
    lon1 -= lon2;
    lon1 *= TO_RAD, lat1 *= TO_RAD, lat2 *= TO_RAD;

    dz = sin(lat1) - sin(lat2);
    dx = cos(lon1) * cos(lat1) - cos(lat2);
    dy = sin(lon1) * cos(lat1);
    double result = asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;

    //convert from KM to meters
    return result * 1000;
}

TEST_CASE("Vector Based"){
    VectorBased vb1;

    CHECK(vb1.fitValue(1, 57.835317, 10.447747,  1000) == 1);
    CHECK(vb1.fitValue(10, 57.835463, 10.499770, 1000) == 1);
    CHECK(vb1.fitValue(20, 57.836762, 10.557729, 1000) == 1);
    CHECK(vb1.fitValue(30, 57.832772, 10.615120, 1000) == 1);
    
    //Shouldn't fit anymore
    CHECK(vb1.fitValue(39, 57.828461, 10.677698, 1000) != 1);

    //Reset and start from the point that didn't fit
    VectorBased vb2;
    CHECK(vb2.fitValue(39, 57.828461, 10.677698, 1000) == 1);
    CHECK(vb2.fitValue(48, 57.816065, 10.735537, 1000) == 1);
    
    //Shouldn't fit anymore
    CHECK(vb2.fitValue(55, 57.794738, 10.783171, 1000) != 1);
    
    //Reset and start from the point that didn't fit
    VectorBased vb3;
    CHECK(vb3.fitValue(55, 57.794738, 10.783171, 1000) == 1);
    CHECK(vb3.fitValue(62, 57.769430, 10.808281, 1000) == 1);
    CHECK(vb3.fitValue(69, 57.750021, 10.833384, 1000) == 1);

    //Shouldn't fit anymore
    CHECK(vb3.fitValue(76, 57.706159, 10.829362, 1000) != 1);
    
    //Reset and start from the point that didn't fit
    VectorBased vb4;
    CHECK(vb4.fitValue(76, 57.706159, 10.829362, 1000) == 1);
}