#ifndef OBSERVER_H
#define OBSERVER_H

class Subject;

class Observer
{
public:
    Observer();
    virtual void notify(Subject* subject) = 0;
};

#endif // OBSERVER_H
