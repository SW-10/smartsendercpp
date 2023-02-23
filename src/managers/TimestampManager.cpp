#include "TimestampManager.h"
#include <iostream>

TimestampManager::TimestampManager(){

}

void TimestampManager::addTimestamp(int timestamp){

    std::cout << "yo" << std::endl;
    timestamps.push_back(timestamp);
    
}