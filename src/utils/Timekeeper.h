#pragma once

#include <string>
#include <iostream>
#include <list>
#include "ObserverPatternInterfaces.h"

class Timekeeper : public ISubject {
private:

    int prev = -1;

    std::list<IObserver *> list_observer_;
    std::string message_;
public:

    void update(const int &timestamp);
    int *intervalSeconds;
    int firstTimestamp = -1;
    virtual ~Timekeeper() {
    }

    /**
 * The subscription management methods.
 */
    void Attach(IObserver *observer) override;

    void Detach(IObserver *observer) override;

    void Notify() override;

    void CreateMessage(std::string message = "Empty");

    void HowManyObserver();
};

