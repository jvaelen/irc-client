#include "socketconnection.h"
#include <QMutex>
// socket headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <cstdio>
#include "sender.h"
#include "receiver.h"

SocketConnection::SocketConnection(ProtocolHandler* handler)
{
    _socketMtx = new QMutex();
    _sender = new Sender(_socketMtx);
    _receiver = new Receiver(handler, _socketMtx);
}

void SocketConnection::stop()
{
    // request a stop
    _receiver->reqStop();
    _sender->reqStop();
    /* wait for the actual stop, this will block indefinitly. It is still possible that while the wait() is called,
        SocketConnection::sendMsg is still called which is not a problem as _transceiver knows it has to stop,
        and will not send any new messages anymore to the socket
        (eventhough this would not be a problem it's an effiency thing)
    */
    _receiver->wait();
    _sender->wait();
    // close the socket
    close(_socket);
}

SocketConnection::~SocketConnection()
{
    stop();
    // clean up
    delete _receiver;
    delete _sender;
    delete _socketMtx;
}

void SocketConnection::sendMsg(const string &msg)
{
    /* its still possible that when the destructor is called, a new message arrives at the same time and handleMessage is
       called thus this makes it possible that a new reply is generated and
       this function is called, because the destructor will
       wait() for the run() function to finished before _transceiver is destructed, this is not a problem. */
    _sender->sendMsg(msg);
}

int SocketConnection::start(const string &address, const string &port)
{
    int rv;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints,0,sizeof hints);
    // only ipv4
    hints.ai_family = AF_INET;
    // irc uses TCP
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(address.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        cerr << "getaddrinfo" << gai_strerror(rv);
        return 1;
    }

    // try to connect to the server
    for (p = servinfo; p; p = p->ai_next) {
        // skip all connections that are not ipv4
        if (p->ai_family != AF_INET)
            continue;
        if ((_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (connect(_socket, p->ai_addr, p->ai_addrlen) == -1) {
            // close the socket that has been created by socket() function because it has failed to connect
            close(_socket);
            return 2;
            continue;
        }
        break;
    }

    if (p == NULL) {
        return 3;
    }

    freeaddrinfo(servinfo);
    _sender->setSocket(_socket);
    _receiver->setSocket(_socket);

    // start both
    _sender->start();
    _receiver->start();

    // started successfully
    return 0;
}
