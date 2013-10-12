#include "sender.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <cstdio>
#include <iostream>
#include <signal.h>

Sender::Sender(QMutex* recvSendMtx) : _msgCount(0),  _shouldStop(false), _recvSendMtx(recvSendMtx)
{
}

void Sender::run()
{
    _connected = true;

    _shouldStopMtx.lock();
    _shouldStop = false;
    _shouldStopMtx.unlock();

    // if the sender is restarted, clear all the messages that where requested to be sent
    _queueMtx.lock(); // we are going to work on the queue
    while(!_msgQueue.empty())
        _msgQueue.pop();
    _queueMtx.unlock();

    // ignore the ipc in this thread
    struct sigaction newAc;
    newAc.sa_handler = SIG_IGN;
    sigemptyset (&newAc.sa_mask); // init
    newAc.sa_flags = 0;
    sigaction(SIGPIPE, &newAc, NULL);

    string temp;
    while ( !threadShouldStop() && _connected ) {
        temp = "";

        if (threadShouldStop())
            return ;
        _msgCount.acquire(1);
        _queueMtx.lock(); // we are going to work on the queue

        /* send is probably slower than getting the string, so we catch it in a temp variable and send it after the lock
         * that way we can keep the time spent @ critical section smaller
         * if we would not do this, there could be a delay on sending a msg (putting it in the queue) because the mutex is stil locked
         * while the msg is being sent
         */

        // it is possible that the semaphore was incremented because of a request to stop, in which case, the messagequeue could be empty
        if (!_msgQueue.empty()) {
            temp = _msgQueue.front();
            _msgQueue.pop();
        }
        _queueMtx.unlock();

        // only send message if the message was actually fetched from the msg queue
        if (!temp.empty()) {
            _recvSendMtx->lock();
            if (send(_socket, temp.c_str(), temp.size(), 0) == -1) {// send the top msg in the queue
                if (errno != EPIPE) {
                    cerr <<  errno << ": " << strerror(errno) << endl;
                    exit(1);
                }
                else // connection closed
                    _connected = false;
            }
            _recvSendMtx->unlock();
        }
    }
}

void Sender::sendMsg(const string& msg)
{
    _queueMtx.lock(); // we are going to work on the queue
    _msgQueue.push(msg);
    _msgCount.release(1); // add 1 to the semaphore
    _queueMtx.unlock();
}

bool Sender::threadShouldStop()
{
    bool temp;
    _shouldStopMtx.lock();
    temp = _shouldStop;
    _shouldStopMtx.unlock();
    return temp;
}

void Sender::reqStop()
{
    _msgCount.release(1); // fake an incoming msg so run() does not block on aquire()
    _shouldStopMtx.lock();
    _shouldStop = true;
    _shouldStopMtx.unlock();
}
