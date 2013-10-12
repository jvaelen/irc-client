#include "dccreceiver.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

DCCReceiver::DCCReceiver(const string &path, const User &user, string ip, string port, long fileSize) : DCC(path, user)
{
    _totalBytes = fileSize;
    _ip = ip;
    _port = port;
}

void DCCReceiver::run()
{
    connectToUser();
}

void DCCReceiver::connectToUser()
{
    int rv;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(_ip.c_str(),_port.c_str(), &hints, &servinfo)) != 0) {
        setStatus(gai_strerror(rv));
        return;
    }

    for(p = servinfo; p; p = p->ai_next){
        if((_sock = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1) {
            setStatus(strerror(errno));
            continue;
        }
        if (::connect(_sock,p->ai_addr,p->ai_addrlen) == -1) {
            close(_sock);
            setStatus(strerror(errno));
            continue;
        }
        break;
    }

    if(p == NULL) {
        setStatus("failed to connect");
        return ;
    }
    else
        setStatus("connected");

    freeaddrinfo(servinfo);

    receive();
}

void DCCReceiver::receive()
{
    char buff[BUFFLEN]; // send the data in chuncks of BUFFLEN byte
    int stop = 0;

    setStatus("receiving");
    ofstream outfile (_path.c_str(),ios::binary | ios::ate);

    do {
        memset(buff, 0, 100); // init buff
        if((stop = recv(_sock, buff, 100, 0)) == -1) {
            setStatus(strerror(errno));
            return;
        }
        incrementSentRecvBytes(stop);
        outfile.write(buff, stop);
    } while (stop != 0&& !isShouldStop());


    setStatus("complete");
    _sentRecvLock.lock();
    _sentRecvBytes = getTotalBytes();
    _sentRecvLock.unlock();
    _stopLock.lock();
    _stopped  = true;
    _stopLock.unlock();
    close(_sock);
    outfile.close();
    notifyObservers();
}
