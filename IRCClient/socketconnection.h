/* Author: Jeroen Vaelen and Balazs Nemeth
 * Description: threadsafe class that is in charge of setting up a network communication link with some address on some port
 */

#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#include <iostream>
#include <string>

#include <QMutex>

#ifdef __linux__
#include <inttypes.h>
#endif

// sender and receiver each have their own thread and share the mutex on the socket because sockets down allow reading/writing at the same time
class Sender;
class Receiver;

class ProtocolHandler;

using namespace std;

/* !!! it is extremely important not to call sendMsg when de destructor
    has been called for SocketConnection !!! */
class SocketConnection
{
public:
    SocketConnection(ProtocolHandler* handler);
    ~SocketConnection();
    // inits the connection, starts it, and creates two other threads: a listener thread to listen to incoming msgs and a sender thread to send msgs
    int start(const string& address, const string& port);
    // stops the connection
    void stop();
    // used when a msg is sent to the server to pass it to the Server thread (object)
    void sendMsg(const string& msg);
private:
    int _socket;
    // for transmitting and receiving data
    Sender* _sender;
    Receiver* _receiver;
    QMutex* _socketMtx;
};

#endif // SOCKETCONNECTION_H
