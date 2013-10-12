#ifndef DCC_H
#define DCC_H

#include "user.h"
#include "subject.h"
#include <inttypes.h>
#include <string>
#include <QThread>
#include <QMutex>

using namespace std;

class IRCClient;

class DCC : public QThread, public Subject
{
public:
    DCC(const string& path, const User& user);

    // notify the thread that it should stop working
    void reqStop();
    // returns true if the thread was stopped (by reqStop) or when the DCCSender is actually done
    bool getStopped();
    string getPath() const {return _path;}
    /* returns the user that has been set in the constructor, this is useful because the DCCSender's accept
        function is only started after the message has been sent to the given user */
    User getUser() const { return _user; }
    // if the path is e.g. /usr/bin/file then this function returns file
    string getFileName() const;
    string getStatus();

    unsigned long getTotalBytes();
    unsigned long getSentRecvBytes();

protected:
    bool isShouldStop();
    // implemented in children
    void run() = 0;

    // path to the file - at sender where to get it from, at receiver where to save it
    string _path;
    // user to send the file to/ to get the file from
    User _user;
    // status of the DCCSender
    string _status;

    // at dccsender this will be the total number of bytes that have been send, at reciever the total bytes that have been received
    uint64_t _sentRecvBytes;
    // total file size
    uint64_t _totalBytes;

    // thread should stop
    bool _shouldStop;
    // thread has stopped
    bool _stopped;

    // set total bytes -- thread safe
    void setTotalBytes(uint64_t bytes);
    // set sent bytes -- thread safe
    void incrementSentRecvBytes(uint64_t bytes);
    // set status -- thread safe
    void setStatus(const string& stat);

    QMutex _stopLock;
    QMutex _sentRecvLock;
    QMutex _totalLock;
    QMutex _statusLock;

};

#endif // DCC_H
