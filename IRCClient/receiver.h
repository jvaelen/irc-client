/* Author: Jeroen Vaelen (and Balazs Nemeth for a little extra functionality and bugfixes)
 * Description: class responsible for recieving messages over a different thread
 */

#ifndef RECEIVER_H
#define RECEIVER_H

#include <QThread>
#include <QMutex>
// needed for select and for fd sets
#include <sys/select.h>

class ProtocolHandler;

class Receiver : public QThread
{
public:
    // mutex that is passed so that there is mutual exclusion when the socket is written and read from
    Receiver(ProtocolHandler* handler, QMutex *mtx);

    void run();

    void setSocket(int socket);

    // request a stop, keep in mind that the request for a stop should be followed by a wait()
    void reqStop();

private:
    int _socket;

    ProtocolHandler* _handler;

    // the socketconnection lets the thread know when it should stop, the thread tries to stop it's exec asap when it gets this
    QMutex _shouldStopMtx;
    bool _shouldStop;
    bool threadShouldStop();
    // will be set to false when the connection closes and the run() function will terminate
    bool _connected;
    // used for timeout interval
    struct timeval _tv;
    // used to interupt blocking select to check if the running thread should be stopped
    fd_set _originalfds;
    // make sure we don't send() and recv() at the same time over the same socket, passed to Reciever and Sender
    QMutex *_recvSendMtx;
};

#endif // RECEIVER_H
