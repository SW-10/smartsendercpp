#include <vector>
#include <utility>
#include <map>

class TimestampManager{
private:
    std::vector<int> timestamps;
    int firstTimestamp;
    int timestampCurrent;
    int timestampPrevious;
    bool readyForOffset = false;

    std::vector<std::pair<int, int>> offsetList;
    int currentOffset;
    std::map<int, int> offsets;
public:
    TimestampManager();
    std::vector<std::pair<int, int>> getOffsetList(){ return offsetList; }
    int getFirstTimestamp(){ return firstTimestamp; }
    void compressTimestamps(int timestamp);
    void reconstructTimestamps();
    bool calcIndexRangeFromTimestamps(int first, int second, int& first_out, int& second_out);
};