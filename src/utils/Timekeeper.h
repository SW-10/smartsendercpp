#pragma once

#include <string>
#include <iostream>
#include <list>
#include "ObserverPatternInterfaces.h"

class Timekeeper : public ISubject {
private:
    int firstTimestamp = -1;
    int prev = -1;
    int intervalSeconds = 3600 * 24;
    std::list<IObserver *> list_observer_;
    std::string message_;
public:

    void update(const int &timestamp);

    virtual ~Timekeeper() {
        std::cout << "bye " << std::endl;
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

