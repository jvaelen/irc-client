#include "ircclient.h"
#include "ircinmessage.h"
#include "ircoutmessage.h"
#include <cassert>
#include <QDebug>
#include <sstream>

#include "channel.h"

string IRCClient::crnl = "\r\n";

IRCClient::IRCClient() :ProtocolHandler(), _socketCon(this)
{
    // default nickname
    _nick = "Baloen";
    _realName = "Unknown";
    _connected = false;
    _hideWhoIsInfo = false;
    /* longest message can be 512, the buffer for handeling the message will thus reserve 512 bytes,
        stdstrings are NOT zero terminated so no extra byte needed */
    _msgBuffer.reserve(2*512);
    addToLastMsgs(TextMessage("Welcome to Baloen IRC Client.", TextMessage::INFO));
    addToLastMsgs(TextMessage("Use /help to see the list of available commands.", TextMessage::INFO));
    addToLastMsgs(TextMessage("Richtclick on users to get some options like send file.", TextMessage::INFO));
}

int IRCClient::connect(const string &serverAddr, const string &port)
{
    if (isConnected())
        disconnect();
    // should return 0 on success
    if (_socketCon.start(serverAddr, port)) {
        addToLastMsgs(TextMessage("failed to connect to " + serverAddr + ":" + port, TextMessage::ERROR));
        return 1;
    }
    // set the connected state to true
    setConnected();
    sleep(1);
    changeNick(_nick);
    sendUserMsg();

    // success!
    return 0;
}

void IRCClient::disconnect()
{
    if (!isConnected())
        return ;

    // quit cleanly
    sendMsg("/quit");
    _socketCon.stop();
    setConnected(false);
}

void IRCClient::setConnected(bool connected)
{
    bool changed = false;
    _isConnectedMtx.lock();
    // only notify observers if the connection changeds
    if (connected != _connected) {
        _connected = connected;
        changed = true;
    }
    _isConnectedMtx.unlock();
    if (connected)
        leaveAllChans();
    if (changed)
        notifyObservers();
}

void IRCClient::leaveAllChans()
{
    _chanMtx.lock();
    while (!_channels.empty()) {
        delete _channels.front();
        _channels.pop_front();
    }
    _chanMtx.unlock();
}

bool IRCClient::isConnected()
{
    bool temp;
    _isConnectedMtx.lock();
    temp = _connected;
    _isConnectedMtx.unlock();
    return temp;
}

void IRCClient::sendMsg(const string &message)
{
    IRCOutMessage outMsg(message);
    if (outMsg.getType() != IRCOutMessage::CONNECT && outMsg.getType() != IRCOutMessage::HELP && !isConnected()) {
        addToLastMsgs(TextMessage("not connected, use \"/connect server:port\" or \"/connect server\"", TextMessage::ERROR));
        return ;
    }
    // irc protocol stuff
    switch (outMsg.getType()){
        case IRCOutMessage::UNSUPPORTED: {
            addToLastMsgs(TextMessage("unknow command: " + outMsg.getParam(), TextMessage::ERROR));
            notifyObservers();
            break;
        }
        case IRCOutMessage::CONNECT: {
            string servAddr = outMsg.getParam();
            if (servAddr.empty()) {
                addToLastMsgs(TextMessage("unknow command: " + outMsg.getParam(), TextMessage::ERROR));
                notifyObservers();
            }
            else {
                int pos;
                    if ((pos = servAddr.find(':')) == (int)string::npos)
                        connect(servAddr, "6667");
                    else
                        connect(servAddr.substr(0, pos), servAddr.substr(pos));
            }
            break;
        }
        case IRCOutMessage::DISCONNECT: {
            disconnect();
            break;
        }
        case IRCOutMessage::NICK: {
            changeNick(outMsg.getParam());
            break;
        }
        case IRCOutMessage::JOIN: {
            _socketCon.sendMsg("JOIN " + outMsg.getParam() + crnl);
            break;
        }
        case IRCOutMessage::PRIVMSG: {
            _socketCon.sendMsg("PRIVMSG " + outMsg.getParam() + " :" + outMsg.getLastParams() + crnl);
            break;
        }
        case IRCOutMessage::INVITE: {
            _socketCon.sendMsg("INVITE " + outMsg.getParam() + " " + outMsg.getLastParams() + crnl);
            break;
        }
        case IRCOutMessage::PART: {
            if (outMsg.getParam()[0] == '#') // only if leaving channel, not private chat
                _socketCon.sendMsg("PART " + outMsg.getParam() + crnl);
            else
                leaveChan(outMsg.getParam());
            break;
        }
        case IRCOutMessage::WHOIS: {
            _socketCon.sendMsg("WHOIS " + outMsg.getParam() + crnl);
            break;
        }
        case IRCOutMessage::TOPIC: {
            // sending /topic #chanName is not for settings the topic
            if (!outMsg.getLastParams().empty())
                _socketCon.sendMsg("TOPIC " + outMsg.getParam() + " :" + outMsg.getLastParams() + crnl);
            break;
        }
        case IRCOutMessage::KICK: {
            _socketCon.sendMsg("KICK " + outMsg.getParam() + " " + outMsg.getLastParams() + crnl);
             break;
        }
        case IRCOutMessage::MODE: {
            _socketCon.sendMsg("MODE " + outMsg.getParam() + " " + outMsg.getLastParams() + crnl);
            break;
        }
        case IRCOutMessage::QUIT: {
        if (outMsg.getParam().empty())
            _socketCon.sendMsg("QUIT" + crnl);
        else
            _socketCon.sendMsg("QUIT :" + outMsg.getParam() + crnl);
            break;
        }
        case IRCOutMessage::HELP: {
            addToLastMsgsWoNotify(TextMessage("available commands:", TextMessage::INFO));
            addToLastMsgsWoNotify(TextMessage("===================", TextMessage::INFO));
            vector<TextMessage> helpCommands;
            IRCOutMessage::getHelp(helpCommands);
            for (unsigned i = 0; i < helpCommands.size(); ++i)
                addToLastMsgsWoNotify(helpCommands[i]);
            notifyObservers();
            break;
        }
        default:
        ;
    }
}

int IRCClient::findcrln(const string& message, unsigned offset)
{
    assert(offset < message.size() - 1);
    // try to find the position of the crnl

    for (unsigned i = offset; i < message.size() - 1; ++i)
        if (message[i] == crnl[0] && message[i + 1] == crnl[1])
            return i;
    // return -1 if the string has not been found
    return -1;
}

void IRCClient::parseInMsg(const string& message)
{
    // print message for dbg
    cout << message << endl;
    // the message passed to this function will be a valid IRC message and will end \r\n
    IRCInMessage ircMessage(message);

    switch (ircMessage.getType()) {
        // awnser the ping with a pong
        case IRCInMessage::PING: {
            _socketCon.sendMsg("PONG " + ircMessage.getParam(0) + crnl);
            break;
        }
        case IRCInMessage::NOTICE: {
            addToLastMsgs(TextMessage("", ircMessage.getLastParams(2, true)));
            break;
        }
        case IRCInMessage::DCCFILE: {
            cout << "recv file" << endl;
            FileRecvReq fileRecvReq;
            fileRecvReq._fileName = ircMessage.getParam(3);
            if (ircMessage.getParamCount() == 7) // 7 parameters needed to have the filesize included aswell
                fileRecvReq._fileSize = ircMessage.getParam(6).substr(0, ircMessage.getParam(6).size() - 1);
            stringstream ss; ss << DCCSender::ushort2Str(atoi(ircMessage.getParam(5).c_str()));
            fileRecvReq._port = ss.str();
            stringstream ss2; ss2 << DCCSender::uintAddr2Str(atoi(ircMessage.getParam(4).c_str()));
            fileRecvReq._addr = ss2.str();
            fileRecvReq._userName = ircMessage.getPrefix().substr(0, ircMessage.getPrefix().find('!'));

            // add to _fileRecvReqs
            _fileRecvReqsMtx.lock();
            _fileRecvReqs.push_back(fileRecvReq);
            _fileRecvReqsMtx.unlock();
            // notify observers to show popup;
            notifyObservers();
            break;
        }
        case IRCInMessage::PRIVMSG: {
            /* if the "channel" that the message is being sent to is actaully the current user, a new "channel" window opens that servers as a private chat
               in that case the channelname is the user that sent the private message to the current user */
            if (ircMessage.getParam(0) != getNick())
                strToChan(ircMessage.getParam(0))->addRecvedMsg(TextMessage(ircMessage.getPrefix(), ircMessage.getLastParams(2, true)));
            else
                strToChan(TextMessage::parseNick(ircMessage.getPrefix()))->addRecvedMsg(TextMessage(ircMessage.getPrefix(), ircMessage.getLastParams(2, true)));
            break;
        }
        case IRCInMessage::RPL_WHOISHOST: {
            if ( getNick() == ircMessage.getParam(0))
                setLocalIP(ircMessage.getParam(3));
            if (!shouldHideInfo())
                addToLastMsgs(TextMessage("", ircMessage.getParam(0) + "'s ip address: " + ircMessage.getParam(3), TextMessage::INFO));
            break;
        }
        case IRCInMessage::ERROR:{
            addToLastMsgs(TextMessage ("",ircMessage.getLastParams(2), TextMessage::ERROR));
            break;
        }
        case IRCInMessage::ERR_CHANOPRIVSNEEDED: {
            strToChan(ircMessage.getParam(1))->addRecvedMsg(TextMessage("", ircMessage.getLastParams(3, true), TextMessage::ERROR));
            break;
        }
        case IRCInMessage::ERR_NOTEXTTOSEND: {
            addToLastMsgs(TextMessage("", ircMessage.getLastParams(2, true), TextMessage::ERROR));
            break;
        }
        case IRCInMessage::KICK: {
            if (ircMessage.getParam(1) == getNick()) {
                addToLastMsgs(TextMessage("", "you have been kicked from channel " + ircMessage.getParam(0) + " [" + ircMessage.getLastParams(3, true) + "]", TextMessage::INFO));
                // still have to actually leave the channel locally
                leaveChan(ircMessage.getParam(0));
            }
            else {
                addToLastMsgs(TextMessage("", "user " + ircMessage.getParam(1) + " has been kicked from " + ircMessage.getParam(0)+ "[" + ircMessage.getLastParams(3, true) + "]", TextMessage::INFO));
                strToChan(ircMessage.getParam(0))->remUser(ircMessage.getParam(1));
            }
            break;
        }
        case IRCInMessage::ERR_NOSUCHNICK:{
            addToLastMsgs(TextMessage("", ircMessage.getParam(1) + " [" + ircMessage.getLastParams(4, true) + "]", TextMessage::ERROR));
            break;
        }
        case IRCInMessage::RPL_ENDOFWHOIS: {
            // this message indicates the end of the whois, which means that the following whois commands can be shown again
            disableHideInfo();
            break;
        }
        case IRCInMessage::RPL_WHOISUSER: {
            if (!shouldHideInfo())
                addToLastMsgs(TextMessage("", ircMessage.getParam(1) + "'s realname: " + ircMessage.getLastParams(5, true), TextMessage::INFO));
            break;
        }
        case IRCInMessage::INVITE: {
            if (ircMessage.getParam(0) == getNick())
                addToLastMsgs(TextMessage("", "you have been invited to channel " + ircMessage.getParam(1) , TextMessage::INFO));
            break;
        }
        case IRCInMessage::RPL_WHOISSERVER: {
            if (!shouldHideInfo()) {
                addToLastMsgs(TextMessage("", ircMessage.getParam(1) + "'s server: " + ircMessage.getParam(2), TextMessage::INFO));
                addToLastMsgs(TextMessage("", ircMessage.getParam(1) + "'s server-info: [" + ircMessage.getLastParams(3, true) + "]", TextMessage::INFO));
            }
            break;
        }
        case IRCInMessage::RPL_WHOISOPERATOR: {
            if (!shouldHideInfo())
                addToLastMsgs(TextMessage("", ircMessage.getParam(1) + " is an operator", TextMessage::INFO));
            break;
        }

        case IRCInMessage::RPL_WHOISCHANNELS: {
            if (!shouldHideInfo())
                addToLastMsgs(TextMessage("", ircMessage.getParam(1) + "'s channels: " + ircMessage.getLastParams(3, true), TextMessage::INFO));
            break;
        }
        case IRCInMessage::RPL_TOPIC: {
            strToChan(ircMessage.getParam(1))->setTopic(ircMessage.getLastParams(3, true));
            break;
        }
        case IRCInMessage::ERR_NICKNAMEINUSE: {
            addToLastMsgs(TextMessage(ircMessage.getLastParams(3, true), TextMessage::ERROR));
            addToLastMsgs(TextMessage("", "chaning nick to " + getNick() + "_", TextMessage::INFO));
            changeNick(getNick() + "_");
            break;
        }
        case IRCInMessage::RPL_MOTD: {
            addToLastMsgs(TextMessage("", ircMessage.getLastParams(2, true)));
            break;
        }
        case IRCInMessage::RPL_NAMREPLY: {
            // add all users to the channel
            for (unsigned i = 3; i < ircMessage.getParamCount(); ++i)
                strToChan(ircMessage.getParam(2))->addUser(User(ircMessage.getParam(i)));
            break;
        }
        case IRCInMessage::USERJOIN: {
            string joinedUser = TextMessage::parseNick(ircMessage.getPrefix());
            if (joinedUser != getNick()) { // if other then the current user joined the channel
                strToChan(ircMessage.getParam(0))->addUser(joinedUser);
                strToChan(ircMessage.getParam(0))->addRecvedMsg(TextMessage("", joinedUser + " has joined " + ircMessage.getParam(0), TextMessage::INFO));
            }
            break;
        }
        case IRCInMessage::USERPART: {
            string partedUser = TextMessage::parseNick(ircMessage.getPrefix());
            if (partedUser != getNick()) { // if other then the current user joined the channel
                strToChan(ircMessage.getParam(0))->remUser(partedUser);
                strToChan(ircMessage.getParam(0))->addRecvedMsg(TextMessage("", partedUser + " has left " + ircMessage.getParam(0), TextMessage::INFO));
            }
            else
                leaveChan(ircMessage.getParam(0)); // the users leaves the channel
            break;
        }
        case IRCInMessage::USERQUIT: {
            string partedUser = TextMessage::parseNick(ircMessage.getPrefix());
            if (partedUser != getNick()) { // if other then the current user quit the server
                remUserFromAllChannels(partedUser);
                addToLastMsgs(TextMessage("", partedUser + " has quit with reason [" + ircMessage.getLastParams(1, true) + "]", TextMessage::INFO));
            }
            break;
        }
        case IRCInMessage::RPL_NOTOPIC: {
            strToChan(ircMessage.getParam(1))->setTopic(ircMessage.getLastParams(3, true));
            break;
        }
        case IRCInMessage::TOPIC: {
            strToChan(ircMessage.getParam(0))->setTopic(ircMessage.getLastParams(2, true));
            break;
        }
        default:
        break;
    }
}

list<IRCClient::FileRecvReq> IRCClient::getFileReqs()
{
    list<FileRecvReq> reqs;
    // add to _fileRecvReqs
    _fileRecvReqsMtx.lock();
    reqs = _fileRecvReqs;
    _fileRecvReqs.clear();
    _fileRecvReqsMtx.unlock();
    return reqs;
}

bool IRCClient::shouldHideInfo()
{
    bool temp;
    _hideWhoIsInfoMtx.lock();
    temp = _hideWhoIsInfo;
    _hideWhoIsInfoMtx.unlock();
    return temp;
}

void IRCClient::disableHideInfo()
{
    _hideWhoIsInfoMtx.lock();
    _hideWhoIsInfo = false;
    _hideWhoIsInfoMtx.unlock();
}

void IRCClient::reqLocalIP()
{
    /* assume that the local nick doesn't change untill a reply is
        requested --> should be easy fix to only send nick change after the reply has been received */
    _hideWhoIsInfoMtx.lock();
    if (getLocalIP().empty()) { // only if the local ip has not been set, assume that onces it's set, it doesn't change
        _socketCon.sendMsg("WHOIS " + getNick() + crnl);
        _hideWhoIsInfo = true;
    }
    _hideWhoIsInfoMtx.unlock();
}

void IRCClient::setLocalIP(const string& ip)
{
    cout << "setting local ip" << endl;
    _localIPMtx.lock();
    _localIP = ip;
    _localIPMtx.unlock();
    // now that the ip is known, send a message for each of the waiting DCC's and move them to running

    _runningDccMtx.lock();
    _waitingDccMtx.lock();

    // move the waiting list into the running list
    while (!_waitingDcc.empty()) {
        sendMessageForDCC(_waitingDcc.front(), ip);
        _waitingDcc.front()->start();
        _runningDcc.push_back(_waitingDcc.front());
        _waitingDcc.pop_front();
    }

    _waitingDccMtx.unlock();
    _runningDccMtx.unlock();
}

string IRCClient::getLocalIP()
{
    string temp;
    _localIPMtx.lock();
    temp = _localIP;
    _localIPMtx.unlock();
    return temp;
}

list<Channel*> IRCClient::getChannels()
{
    list<Channel*> temp;
    _chanMtx.lock();
    temp = _channels;
    _chanMtx.unlock();
    return temp;
}

void IRCClient::leaveChan(const string& chanName)
{
    _chanMtx.lock();
    bool found = false;
    for (list<Channel*>::iterator i = _channels.begin(); i != _channels.end() && !found; ++i) {
        if ((found = ((*i)->getName() == chanName))) {
            delete*i;
            // remove that element from the list
            _channels.erase(i);
        }
    }
    _chanMtx.unlock();
    // this should ALWAYS hold as the user can only leave a channel (or private chat with a user) on which he is
    assert(found);
    notifyObservers();
}

void IRCClient::remUserFromAllChannels(const string& user)
{
    _chanMtx.lock();
    for (list<Channel*>::iterator i = _channels.begin(); i != _channels.end(); ++i) {
        (*i)->remUser(User(user));
    }
    _chanMtx.unlock();
}

Channel* IRCClient::strToChan(const string& chanName)
{
    bool found = false;
    Channel* tempChanP = 0;
    // first try to find the channel
    _chanMtx.lock();
    for (list<Channel*>::iterator i = _channels.begin(); i != _channels.end() && !found; ++i) {
        if ((found = (*i)->getName() == chanName))
            tempChanP =  *i;
    }
    _chanMtx.unlock();
    if (found)
        return tempChanP;

    // create it outside the mutex
    tempChanP = new Channel(this, chanName);
    _chanMtx.lock();
    // if the previous forloop didn't return, the channel didn't exist
    _channels.push_back(tempChanP);
    _chanMtx.unlock();
    // let the observers know that a channel has been added
    notifyObservers();

    // use the pointer because there is no lock on the _channels list at the moment
    return tempChanP;
}

void IRCClient::addToLastMsgs(const TextMessage& textMsg)
{
    addToLastMsgsWoNotify(textMsg);
    notifyObservers();
}

void IRCClient::addToLastMsgsWoNotify(const TextMessage& textMsg)
{
    _chanMtx.lock();
    for (list<Channel*>::iterator i = _channels.begin(); i != _channels.end(); ++i) {
        (*i)->addRecvedMsg(textMsg);
    }
    _chanMtx.unlock();
    _lastMsgsMtx.lock();
    _lastMsgs.push(textMsg);
    _lastMsgsMtx.unlock();
}


void IRCClient::handleMsg(const string &message)
{
    // messages can be received in pieces which have to be stiched together again
    // add to buffer
    _msgBuffer += message;
    unsigned begin = 0, pos = 0;
    int lastPos = 0;
    // _msgBuffer will hold something like "message \r\n message \r\n messa" with the last message incomplete for example
    // (size_t) -1 is the largest integral value for the unsigned values representable by size_t
    while(begin != _msgBuffer.size() && (pos = _msgBuffer.find(crnl, begin)) != -1) {
        // parse message seperatly
        parseInMsg(_msgBuffer.substr(begin, pos + crnl.size() - begin));
        begin = pos + crnl.size();
        lastPos = pos;
    }

    // save the rest of the message which is the beginning of the next message. If lastPos +2 == _msgBuffer.size(), _msgBuffer will be ""
    _msgBuffer = _msgBuffer.substr(lastPos + 2);

    notifyObservers();
}

queue<TextMessage> IRCClient::getLastMsgs()
{
    queue<TextMessage> tempQueue;
    _lastMsgsMtx.lock();
    while (!_lastMsgs.empty()) {
        tempQueue.push(_lastMsgs.front());
        _lastMsgs.pop();
    }
    _lastMsgsMtx.unlock();
    return tempQueue;
}

DCC* IRCClient::recvFile(const FileRecvReq &fileRecvReq)
{
    DCCReceiver* newDCCRecv = new DCCReceiver(fileRecvReq._path, fileRecvReq._userName, fileRecvReq._addr, fileRecvReq._port, atoi(fileRecvReq._fileSize.c_str()));
     _runningDccMtx.lock();
    _runningDcc.push_back(newDCCRecv);
    _runningDccMtx.unlock();
    newDCCRecv->start();

    return newDCCRecv;
}

DCC* IRCClient::sendFile(const string& filePath, const User& user)
{
    // create new DCC instance with the given path and the user.
    DCCSender* newDCC = new DCCSender(filePath, user);
    string tempAddr;

    tempAddr = getLocalIP();
    // if the local ip has already been set
    if (!(tempAddr.empty())) {

        _runningDccMtx.lock();
        _runningDcc.push_back(newDCC);
        /* send message letting the other side know that this client is
            listening on a given port and is trying to send a file */
        sendMessageForDCC(newDCC, tempAddr);
        newDCC->start();
        _runningDccMtx.unlock();
    }
    else {
        // request the local ip if it is unknown
        reqLocalIP();
        _waitingDccMtx.lock();
        _waitingDcc.push_back(newDCC);
        _waitingDccMtx.unlock();
    }

    return newDCC;
}

void IRCClient::sendMessageForDCC(DCCSender *dcc, const string &localAddr)
{
    string fname = dcc->getFileName();
    string fnamewospaces;
    // filter spaces
    for (unsigned i = 0; i < fname.size(); ++i)
    {
        if (fname[i] != ' ')
            fnamewospaces += fname[i];
    }
    stringstream ss;
    ss << "PRIVMSG ";
    ss << dcc->getUser().getName() << ":" << (char)1 << "DCC SEND " << fnamewospaces + " ";
    ss << DCCSender::strAddr2Uint(localAddr) << " " << DCCSender::strPort2Ushort(dcc->getPort()) << " ";
    ss << dcc->getTotalBytes() << (char)1 << crnl;

    _socketCon.sendMsg(ss.str());
}

void IRCClient::sendUserMsg()
{
    _socketCon.sendMsg("USER " + _nick + "ident" + " 0 * :" + _realName + crnl);
}

void IRCClient::changeNick(const string &newNick)
{
    _nick = newNick;
    _socketCon.sendMsg("NICK " + _nick + crnl);
}
