#include "receiver.h"
#include "socketconnection.h"
#include <cstdio>
#include <string>
#include <QMutex>
// socket includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include <protocolhandler.h>

using std::string;

Receiver::Receiver(ProtocolHandler* handler, QMutex* mtx) : _handler(handler), _recvSendMtx(mtx)
{
    // timeout interval has to be set each iteration because select() changes the values
}

void Receiver::run()
{
    // by default, assume there is a connection
    _connected = true;

    _shouldStopMtx.lock();
    _shouldStop = false;
    _shouldStopMtx.unlock();

    string totalMsg;
    // we get every message in blocks of 1023 bits (+1 for '\0')
    uint8_t msg[1024];
    // holds the number of bytes received or the return value of select
    int retVal;
    // original fdset will be copied into this because select changes the values inside
    fd_set readfds;

    while ( !threadShouldStop() && _connected) {
        /* the tv_usec has to be set always because select changes
            the value to how long it would have waited if the socket wasn't ready to read */
        _tv.tv_usec = 500000;
        _tv.tv_sec = 0;
        /* we are going to use Synchronous I/O Multiplexing to check when we can recv from the socket
         * we give it 0.5 sec for the socket to receive data, if after 0.5 sec there is no data to read
         * we recheck if we should have to safely end the thread, if we do get data we get it from recv */

        // make a copy of the original fd set
        readfds = _originalfds;
        // give _socket 0.5 sec to receive data
        select(_socket + 1, &readfds, NULL, NULL, &_tv);
        if (FD_ISSET(_socket, &readfds)) { // there is data to read from _socket, so recv will not block

            // read all the data there is to read, handling one incoming message at a time
            totalMsg = "";

            // don't read/write at the same time over the same file descriptor
            _recvSendMtx->lock();
            if ((retVal = recv(_socket, msg, 1023, 0)) == -1) {
                cerr << strerror(errno) << endl;
                exit(1);
            }
            _recvSendMtx->unlock();
            /* zero terminated string, retVal will either be positive or 0,
                don't check on 0 because there is no need to */
            msg[retVal] = 0;

            if (retVal)
                totalMsg = (const char*)msg;
            else {// if retVal is 0, the connection closed
                _connected = false;
                _handler->setConnected(false);
            }
            // no while loop because it can't be determined if the connection closed if a while loop is used

            // it's only usefull to send a message if _connected which means that there is some data
            if (_connected)
                _handler->handleMsg(totalMsg);
        }
    }
}

void Receiver::setSocket(int socket)
{
    _socket = socket;
    // at this time the fds can be initialized
    FD_ZERO(&_originalfds);
    FD_SET(_socket, &_originalfds);
}

void Receiver::reqStop()
{
    _shouldStopMtx.lock();
    _shouldStop = true;
    _shouldStopMtx.unlock();
}

bool Receiver::threadShouldStop()
{
    bool temp;
    _shouldStopMtx.lock();
    temp = _shouldStop;
    _shouldStopMtx.unlock();
    return temp;
}
