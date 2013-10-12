#include "dccsender.h"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sstream>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <errno.h>

DCCSender::DCCSender(const string& path, const User& user) : DCC(path,user)
{
    // open file and get filesize
    _is.open (_path.c_str(), ios::binary );
    // get length of file
    _is.seekg (0, ios::end);
    setTotalBytes(_is.tellg());
    _is.seekg (0, ios::beg);

    // start listening immediatly
    openAndListen();
}

int DCCSender::openAndListen()
{
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    int rv;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, "0", &hints, &servinfo)) != 0) {
        setStatus(gai_strerror(rv));
        return 2;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p; p = p->ai_next) {

        // create new socket
        if ((_listenSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue; // new iteration

        // set socket options
        if (setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            setStatus(strerror(errno));
            return 1;
        }

        // bind socket to port
        if (bind(_listenSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(_listenSocket);
            return 1;
            continue; // new iteration
        }

        break; // if we get here, stop and go back to for-loop
    }

    if (p == NULL)
    {
        setStatus("failed to bind");
        return 2; //"server: failed to bind\n"
    }

    freeaddrinfo(servinfo); // all done with this structure

    // now get the actual port
    struct sockaddr_in portFetch;
    socklen_t sizeOfAddrInfo = sizeof portFetch;
    getsockname(_listenSocket, (struct sockaddr *)&portFetch, &sizeOfAddrInfo);
    stringstream ss;
    ss << ntohs(portFetch.sin_port);
    _port = ss.str();


    // ready to accept incoming requests
    if (listen(_listenSocket, 1) == -1) {
        setStatus("failed to listen");
        return 1;
    }

    setStatus("waiting for ip");

    // success
    return 0;
}

string DCCSender::uintAddr2Str(uint32_t ipaddr)
{
    return inet_ntoa(*(in_addr*)&ipaddr);
}

string DCCSender::ushort2Str(uint16_t port)
{
    stringstream ss;
    ss << ntohs(port);
    return ss.str();
}

uint32_t DCCSender::strAddr2Uint(const string& ipaddr)
{
    uint32_t temp;
    inet_aton(ipaddr.c_str(), (in_addr*)&(temp));
    return temp;
}

unsigned DCCSender::strPort2Ushort(const string& port)
{
    return htons(atoi(port.c_str()));
}

void DCCSender::run()
{
    setStatus("waiting for connection");
    struct sockaddr_storage their_addr; // connector's address info
    socklen_t sin_size;
    sin_size = sizeof their_addr;
    bool success = false;
    int i = 0;

    struct timeval tv;
    fd_set readfds;

    // every second we increment i with 1, the other user has 1 minute to connect to our listening socket
    do {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(_listenSocket, &readfds);

        select(_listenSocket+1, &readfds, NULL, NULL, &tv);

        // we can accept
        if (FD_ISSET(_listenSocket, &readfds)) {
            _dataSocket = accept(_listenSocket, (struct sockaddr*)&their_addr, &sin_size);
            success = true;
            // close the passive socket
            close(_listenSocket);
        }
        sleep(1);

        i++;

    } while ( i < 60 && !success  && !isShouldStop());

    if (isShouldStop()) {
        _stopped = true;
        notifyObservers();
    }
    else {
        // start sending the data
        if (success) {
            setStatus("sending");
            sendData();
        }
        else
            setStatus("timed out");
    }
}

void DCCSender::sendData()
{
    char buffer[BUFLEN];

    ssize_t sent;

    while (_is.read(buffer, BUFLEN) && !isShouldStop())
    {
        if ( (sent = send(_dataSocket, buffer, _is.gcount(), 0)) == -1) {
            setStatus(strerror(errno));
            return ;
        }
        incrementSentRecvBytes(sent);
    }

    if (_is.eof())
    {
        if (_is.gcount() > 0)
        {
            if ( (sent = send(_dataSocket, buffer, _is.gcount(), 0)) == -1) {
                setStatus(strerror(errno));
                return ;
            }
            incrementSentRecvBytes(sent);
        }
    }
    else if (_is.bad())
        setStatus("errror reading");

    _is.close();
    close(_dataSocket);

    if (isShouldStop())
        setStatus("prematurely terminated");
    else
        setStatus("completed");


    _sentRecvLock.lock();
    _sentRecvBytes = getTotalBytes();
    _sentRecvLock.unlock();

    _stopLock.lock();
    _stopped = true;
    _stopLock.unlock();
    notifyObservers();
}
