#pragma once

#include <string>

// Observer pattern
// Src: https://refactoring.guru/design-patterns/observer/cpp/example

class IObserver {
public:
    virtual ~IObserver() {};

    virtual void Update(const std::string &message_from_subject) = 0;
};

class ISubject {
public:
    virtual ~ISubject() {};

    virtual void Attach(IObserver *observer) = 0;

    virtual void Detach(IObserver *observer) = 0;

    virtual void Notify() = 0;
};
