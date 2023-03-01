#include <cstdint>;
#include <vector>;

class ArrowFlightManager{
    int modelTypeId;
    int startTime;
    int endTime;
    float minValue;
    float maxValue;
    std::vector<int> values;
};