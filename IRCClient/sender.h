/* Author: Jeroen Vaelen
 * Description: This class handles sending messages over a SocketConnection.
 *              It is derived from QThread because there is one thread that just sends messages to the serves when necessary
 */

#ifndef SENDER_H
#define SENDER_H

#include <QThread>
#include <string>
#include <queue>
#include <QMutex>
#include <QSemaphore>


using namespace std;

class Sender : public QThread
{
public:
    Sender(QMutex *recvSendMtx);
    // adds the msg to the msgQueue so that it can be schedueled to be sent to the server
    void sendMsg(const string& msg);
    // sets the socket fd
    void setSocket(int socket) { _socket = socket; }

    // the thread should stop its execution
    void reqStop();
protected:
    // actually sends the msg's in the msgqueue over the socket
    void run();
private:
    // passed via SocketConnection
    int _socket;
    // holds all msg that we have to send to the server
    queue<string> _msgQueue;

    // mutex on msgQueue
    QMutex _queueMtx;
    // semaphore to check when we have to wait
    QSemaphore _msgCount;

    // make sure the _shouldStop bool isn't changed by SocketConnection while we are reading from it
    QMutex _shouldStopMtx;
    // we want our thread to stop safely
    bool threadShouldStop();
    bool _shouldStop;
    bool _connected;

    // make sure we don't send() and recv() at the same time over the same socket, passed to Reciever and Sender
    QMutex* _recvSendMtx;

};

#endif // SENDER_H
