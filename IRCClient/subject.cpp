#include "subject.h"
#include <QDebug>
void Subject::registerObserver(Observer* addThis)
{
    _observerCollection.push_back(addThis);
    addThis->notify(this); // let the observer know who his subject is
    /* we could also wait until the first time the subject calls notifyObservers(), but the observer should be able to communicate
     * to his subject after registration, it should not have to wait until the first time notifyObservers() is called */
}

void Subject::registerObservers(list<Observer*> observers)
{
    for (list<Observer*>::iterator i = observers.begin(); i != observers.end(); ++i)
        _observerCollection.push_back(*i);
}

void Subject::unregisterObserver(Observer *removeThis)
{
    // if removeThis exists, remove it (see below)
    // if (find(m_observerCollection.begin(), m_observerCollection.end(), removeThis) != m_observerCollection.end())
    // this function will remove it in linaire time, we don't have to check if it exists
    _observerCollection.remove(removeThis);
}

void Subject::notifyObservers()
{
    unsigned sizeChange = _observerCollection.size();
    // polymorphism to call notify() on all the observers
    list<Observer*>::iterator i = _observerCollection.begin();
    while (i != _observerCollection.end())
    {
        (*i)->notify(this);
        ++i;
        // if the notify caused the observer to unregister itself, the size changes of the list and we reset to the beginning to prevent problems
        if (sizeChange != _observerCollection.size())
        {
            i = _observerCollection.begin();
            sizeChange = _observerCollection.size();
        }
    }
}

void Subject::unregisterAllObservers()
{
    _observerCollection.clear();
}
