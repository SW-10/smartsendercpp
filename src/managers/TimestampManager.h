#include <vector>
#include <utility>

class TimestampManager{
private:
    std::vector<int> timestamps;
    std::vector<std::pair<int, int>> offsetList;
public:
    TimestampManager();

    void addTimestamp(int timestamp);
    
};