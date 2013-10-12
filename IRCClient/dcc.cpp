#include "dcc.h"

DCC::DCC(const string &path, const User &user) : _user(user)
{
    _path = path;
    _shouldStop = false;
    _stopped = false;
    _status = "initializing";
    _sentRecvBytes = 0;
    _totalBytes = 0;
}

void DCC::reqStop()
{
    /* lock mutex, set shouldStop = true unlockMutex */
    _stopLock.lock();
    _shouldStop = true;
    _stopLock.unlock();
}

bool DCC::isShouldStop()
{
    bool temp;
    _stopLock.lock();
    temp = _shouldStop;
    _stopLock.unlock();
    return temp;
}

unsigned long DCC::getSentRecvBytes()
{
    _sentRecvLock.lock();
    uint64_t bytes = _sentRecvBytes;
    _sentRecvLock.unlock();
    return bytes;
}

unsigned long DCC::getTotalBytes()
{
    _totalLock.lock();
    uint64_t bytes = _totalBytes;
    _totalLock.unlock();
    return bytes;
}

void DCC::incrementSentRecvBytes(uint64_t bytes)
{
    _sentRecvLock.lock();
    _sentRecvBytes += bytes;
    _sentRecvLock.unlock();
}

void DCC::setTotalBytes(uint64_t bytes)
{
    _totalLock.lock();
    _totalBytes = bytes;
    _totalLock.unlock();
}

bool DCC::getStopped()
{
    _stopLock.lock();
    bool stopped = _stopped;
    _stopLock.unlock();
    return stopped;
}

void DCC::setStatus(const string& stat)
{
    _statusLock.lock();
    _status = stat;
    _statusLock.unlock();
}

string DCC::getFileName() const
{
    size_t found;
    found = _path.find_last_of("/");
    return _path.substr(found+1);
}

string DCC::getStatus()
{
    _statusLock.lock();
    string stat = _status;
    _statusLock.unlock();
    return stat;
}
