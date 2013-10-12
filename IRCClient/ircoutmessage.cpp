#include "ircoutmessage.h"
#include "ircinmessage.h"

#include <cassert>

// init static members
map<string, IRCOutMessage::MessageType> IRCOutMessage::_messageMap;
bool IRCOutMessage::_initDone = IRCOutMessage::initMap();

IRCOutMessage::IRCOutMessage(const string &message)
{
    size_t pos = message.find(' ');

    string cmd = message.substr(0, pos);

    // _msgType will be returned with getMessageType
    if (_messageMap.count(cmd) > 0)
        _msgType = _messageMap.at(cmd);
    else
        _msgType = UNSUPPORTED;

    // find next space
    size_t pos2 = message.find(' ', pos + 1);
    // either there is just one parameter or there are more (the latter case is true if there are 2 spaces)
    // save the actual message into _params
    if (pos2 != string::npos) {
        _param = message.substr(pos + 1, pos2 - pos - 1);
        _lastParams = message.substr(pos2 + 1, message.size() - 1 - pos2);
        _extraParams = true;
    }
    else {
        _param = message.substr(pos + 1, message.size() - 1 - pos);
        _extraParams = false;
    }
}


void IRCOutMessage::getHelp(vector<TextMessage> &container)
{
    container.reserve(_messageMap.size());
    for(map<string, MessageType>::const_iterator i = _messageMap.begin(); i != _messageMap.end(); ++i)
        container.push_back(TextMessage(i->first, TextMessage::INFO));
}


bool IRCOutMessage::initMap()
{
    _messageMap.insert(pair<string, MessageType>("/join", JOIN));
    _messageMap.insert(pair<string, MessageType>("/invite", INVITE));
    _messageMap.insert(pair<string, MessageType>("/nick", NICK));
    _messageMap.insert(pair<string, MessageType>("/msg", PRIVMSG));
    _messageMap.insert(pair<string, MessageType>("/topic", TOPIC));
    _messageMap.insert(pair<string, MessageType>("/part", PART));
    _messageMap.insert(pair<string, MessageType>("/whois", WHOIS));
    _messageMap.insert(pair<string, MessageType>("/mode", MODE));
    _messageMap.insert(pair<string, MessageType>("/quit", QUIT));
    _messageMap.insert(pair<string, MessageType>("/connect", CONNECT));
    _messageMap.insert(pair<string, MessageType>("/disconnect", DISCONNECT));
    _messageMap.insert(pair<string, MessageType>("/kick", KICK));
    _messageMap.insert(pair<string, MessageType>("/help", HELP));
    return true;
}
