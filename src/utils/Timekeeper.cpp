#include "Timekeeper.h"
#include <iostream>

void Timekeeper::update(const int &timestamp) {
    // Handle first timestamp
    if (firstTimestamp == -1) {
        firstTimestamp = timestamp;
        prev = 0;
    }

    // Emit message if we go into a new interval
    int current = timestamp - firstTimestamp;
    int counter = current % *intervalSeconds;

    if(counter < prev){
        //std::cout << current << std::endl;
        this->message_ = "New interval";
        this->Notify();
    }
    prev = counter;
}


/**
* The subscription management methods.
*/
void Timekeeper::Attach(IObserver *observer) {
    list_observer_.push_back(observer);
}

void Timekeeper::Detach(IObserver *observer) {
    list_observer_.remove(observer);
}

void Timekeeper::Notify() {
    std::list<IObserver *>::iterator iterator = list_observer_.begin();
    // HowManyObserver();
    while (iterator != list_observer_.end()) {
        (*iterator)->Update(message_);
        ++iterator;
    }
}

void Timekeeper::CreateMessage(std::string message) {
    this->message_ = message;
    Notify();
}

void Timekeeper::HowManyObserver() {
    std::cout << "There are " << list_observer_.size() << " observers in the list.\n";
}