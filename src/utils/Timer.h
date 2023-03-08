#pragma once

#include <chrono>
#include <vector>

class Timer{
    bool readyToEnd = false;
    std::vector<std::chrono::high_resolution_clock::time_point> times;
public:
    void begin(){
        times.clear();
        times.push_back(std::chrono::high_resolution_clock::now());
        readyToEnd = true;
    }
    double end(){
        if(readyToEnd){
            readyToEnd = false;
            times.push_back(std::chrono::high_resolution_clock::now());
            return std::chrono::duration<double, std::milli>(times.at(1)-times.at(0)).count();
        } else {
            return 0;
        }
    }
};