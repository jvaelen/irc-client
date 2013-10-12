/* Author: Balazs Nemeth
 * Description: This class handles all irc protocol specific stuff. It constructs the correct messages following the 2812 rfc
 */

#ifndef IRCCLIENT_H
#define IRCCLIENT_H

#include <inttypes.h>
#include <queue>
#include <string>
#include <list>
#include <QMutex>

#include "dcc.h"
#include "dccsender.h"
#include "dccreceiver.h"
#include "subject.h"
#include "protocolhandler.h"
#include "socketconnection.h"
#include "textmessage.h"
#include "channel.h"


class SocketConnection;
using namespace std;

class IRCClient : public ProtocolHandler, public Subject
{
public:
    struct FileRecvReq
    {
        string _fileName;
        string _path;
        string _port;
        string _userName;
        string _fileSize;
        string _addr;
    };

    /* creates an instance of IRCClient, the serveraddr and port have to be given
       which will be delegated to the underlying SocketConnection */
    IRCClient();
    // if the nick has been set, it will be used as the nick when connection to the irc server has been made
    void setNick(const string& nick) { _nick = nick; }
    string getNick() const {return _nick;}
    // sets the real name of the user
    void setRealName(const string& realName) {_realName = realName;}
    /* calling this function will set up the connection using the SocketConnection instance and will return imediatly.
       after connect() has been called, messages can be sent and received form the irc server which has been connected
       connect() will return 0 on sucess, if the connection was refused, it will return 1
    */
    // sends a file using DCC to a given user, returns the DCC instance that has been created to send the specific file
    DCC* sendFile(const string& filePath, const User& user);
    int connect(const string& serverAddr, const string& port = "6667");
    // disconnect
    void disconnect();
    /* the function will be called by the underlying SocketConnection instance and can be seen as a kind of
       callback function which will receive the data from the SocketConnection.
       The function will do protocol specific handling of the data passed to it by the socketConnection */
    void handleMsg(const string& message);

    /* sends a message to the server. The message is parsed for commands
       like /join, /msg, etc (the commands are the onces commonly knownon irc) */
    void sendMsg(const string& message);
    queue<TextMessage> getLastMsgs();
    list<Channel*> getChannels();
    // sets the connected state, can
    void setConnected(bool connected = true);
    // query to see if the connection is still alive
    bool isConnected();
    // clears the list of filerecv requests and returns the list (not const, because it clears the lists)
    list<FileRecvReq> getFileReqs();
    // receive a file
    DCC* recvFile(const FileRecvReq& fileRecvReq);
private:
    // help function for sending the initializing message of the dcc over irc, localAddr passed as parameter for effiency
    void sendMessageForDCC(DCCSender* dcc, const string& localAddr);
    // returns channel, if it doesn't exists, it creates the channel and returns that
    Channel* strToChan(const string& chanName);
    void remUserFromAllChannels(const string& user);
    void leaveAllChans();
    void leaveChan(const string& chanName);
    // adds to the last message queue threadsafely
    void addToLastMsgs(const TextMessage& textMsg);
    void addToLastMsgsWoNotify(const TextMessage& textMsg);
    // parses a string which HAS TO BE a valid irc command, if it is parsed, the message is saved into the queue of messages
    void parseInMsg(const string& message);
    // find the position of \r\n in the string, -1 is returned if it doesn't contain crln.
    int findcrln(const string& message, unsigned offset = 0);
    void changeNick(const string& newNick);
    // sends USER msg immediatly followed after the first NICK opon connection, this is private because it is for initialization
    void sendUserMsg();

    // sends a request for the local ip, a boolean is set so that the next whois information is not shown
    void reqLocalIP();
    bool _hideWhoIsInfo;
    // returns the hideWhoIsInfo threadsafely
    bool shouldHideInfo();
    // sets _hideWhoIsInfo to false
    void disableHideInfo();
    QMutex _hideWhoIsInfoMtx;

    // threadsafe getter and setter for the local ip
    void setLocalIP(const string& ip);
    string getLocalIP();

    // filerequests are put inside this list, if the GUI sees a file request, it will show a popup and handle what the user decides to do
    list<FileRecvReq> _fileRecvReqs;
    QMutex _fileRecvReqsMtx;
    // msg buffer used to stitch together broken string that have been received through the socket in pieces
    string _msgBuffer;
    string _nick;
    string _realName;
    /* string holding the IP address of the current user, initially the string is empty. After one whois about the current user,
       the IP address is saved in the string so that it can be reused with each DCC, it is only set once. If the string is
       empty when a DCC instance is created, a whois is sent silently to fetch the current users ip and the DCC instance
       is put into _waitingDCC. The reply to the whois starts each of the connections after a
       cc message is sent to each of the users*/
    string _localIP;
    QMutex _localIPMtx;
    // \r\n is used so many times (following the rfc in the protocol) so that it has been defined once here
    static string crnl;
    // channels
    QMutex _chanMtx;
    list<Channel*> _channels;

    // if the local ipaddress of the user is not known when the DCC is created, it is added to the waitingDCC list
    list<DCCSender*> _waitingDcc;
    /* once the ip is known, this can be after the ip has been received through a whois for the current user,
        the dcc is moved from the waitingDcc into the runningDcc list. */
    list<DCC*> _runningDcc;
    QMutex _waitingDccMtx;
    QMutex _runningDccMtx;
    // underlying socetconnection which works fully threaded
    SocketConnection _socketCon;

    // temp lists of all the messages received
    queue<TextMessage> _lastMsgs;
    // queue that holds all the error messages
    queue<TextMessage> _lastErrorMsgs;
    QMutex _lastMsgsMtx;

    bool _connected;
    // making the boolean _connected thread safe
    QMutex _isConnectedMtx;
};

#endif // IRCCLIENT_H
