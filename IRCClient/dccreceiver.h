#ifndef DCCRECEIVER_H
#define DCCRECEIVER_H

#include "dcc.h"

#define BUFFLEN 100

class DCCReceiver : public DCC
{
public:
    DCCReceiver(const string& path, const User& user, string ip, string port, long fileSize = -1);

protected:
    void run();

private:
    string _ip;
    string _port;

    int _sock;

    // connects to the user
    void connectToUser();
    // receives the data and writes it to the file at _path
    void receive();

};

#endif // DCCRECEIVER_H
