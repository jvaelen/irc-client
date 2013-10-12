#ifndef DCCSENDER_H
#define DCCSENDER_H

#define BUFLEN 8192

#include "dcc.h"
#include <fstream>

using namespace std;

class IRCClient;

class DCCSender : public DCC
{
public:
    DCCSender(const string& path, const User& user);

    string getPort() const { return _port; }

    static unsigned strAddr2Uint(const string& ipaddr);
    static unsigned strPort2Ushort(const string& port);
    static string uintAddr2Str( uint32_t ipaddr);
    static string ushort2Str(uint16_t port);
protected:
    // accepts the user connecting (he has 1 min before rejection) and sends the data
    void run();

private:
    // the port that is used to listen on
    string _port;
    // socket that is opened when we are listening for the user to accept the data transfer, only used once
    int _listenSocket;
    // the socket that is used for the actual datatransfer
    int _dataSocket;
    // file to read
    ifstream _is;
    // opens new socket and starts listening, returns 0 if succesfully listening
    int openAndListen();
    // called in run() to actuall send the data of the file @ _path
    void sendData();
};

#endif // DCCSENDER_H
