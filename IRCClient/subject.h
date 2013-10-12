#ifndef SUBJECT_H
#define SUBJECT_H

#include <list>
#include <iterator>

#include "observer.h"

using namespace std;

class Subject
{
public:
    // default constructor used to create a Subject
    Subject() { }
    // copy from another observer
    Subject(const Subject& other) { _observerCollection = other._observerCollection; }
    // the following functions are virtual because HybridGraph reimplements them
    // register an oberserver to this subject
    virtual void registerObserver(Observer* observer);
    virtual void registerObservers(list<Observer*> observers);
    // remove one specific observer from this subject
    virtual void unregisterObserver(Observer* observer);
    // removes all the observers (this method does NOT delete them)
    virtual void unregisterAllObservers();
    // calls update() on all it's observers
    virtual void notifyObservers();
    // returns the list of observers so that it can be copied to other subjects
    virtual list<Observer*> getObservers() const {return _observerCollection; }
protected:
    list<Observer*> _observerCollection;
};

#endif // SUBJECT_H
