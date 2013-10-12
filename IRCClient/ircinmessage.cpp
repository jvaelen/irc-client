#include <sstream>
#include <cassert>
#include "ircinmessage.h"

#include <cassert>


map<string, IRCInMessage::MessageType> IRCInMessage::_messageMap;
bool IRCInMessage::_initDone = IRCInMessage::initMap();


IRCInMessage::IRCInMessage(const string &message)
{
    assert(!message.empty());

    stringstream ss(message);

    _hasPrefix = message[0] == ':';

    bool firstToken = true;
    string buf;
    while (ss >> buf) {
        // only for the first token
        if (firstToken && _hasPrefix) {
            firstToken = false;
            _prefix = buf.substr(1, buf.size() - 1);
        }
        else
            _tokens.push_back(buf);
    }
    // no need to remove last \r\n because stringstream does this already
}

IRCInMessage::MessageType IRCInMessage::getType() const
{
    if (_messageMap.count(_tokens[0]) >0)
    {
        MessageType temp = _messageMap.at(_tokens[0]);
        if (temp == PRIVMSG && getParam(1)[0] == 1)
            return DCCFILE;
        else
            return temp;
    }
    else
        return SERVERMESSAGE;
}

string IRCInMessage::getParam(unsigned index) const
{
    assert (index + 1 < _tokens.size());
    if (_tokens[index + 1][0] == ':')
        return _tokens[index + 1].substr(1);
    else
        return _tokens[index + 1];
}


bool IRCInMessage::initMap()
{
    _messageMap.insert(pair<string, MessageType>("PING", PING));
    _messageMap.insert(pair<string, MessageType>("NOTICE", NOTICE));
    _messageMap.insert(pair<string, MessageType>("PRIVMSG", PRIVMSG));
    _messageMap.insert(pair<string, MessageType>("KICK", KICK));
    _messageMap.insert(pair<string, MessageType>("INVITE", INVITE));
    _messageMap.insert(pair<string, MessageType>("TOPIC", TOPIC));
    _messageMap.insert(pair<string, MessageType>("001", SERVERMESSAGE));
    _messageMap.insert(pair<string, MessageType>("002", SERVERMESSAGE));
    _messageMap.insert(pair<string, MessageType>("003", SERVERMESSAGE));
    _messageMap.insert(pair<string, MessageType>("004", SERVERMESSAGE));
    _messageMap.insert(pair<string, MessageType>("005", SERVERMESSAGE));
    _messageMap.insert(pair<string, MessageType>("331", RPL_NOTOPIC));
    _messageMap.insert(pair<string, MessageType>("332", RPL_TOPIC));
    _messageMap.insert(pair<string, MessageType>("353", RPL_NAMREPLY));
    _messageMap.insert(pair<string, MessageType>("372", RPL_MOTD));
    _messageMap.insert(pair<string, MessageType>("433", ERR_NICKNAMEINUSE));
    _messageMap.insert(pair<string, MessageType>("311", RPL_WHOISUSER));
    _messageMap.insert(pair<string, MessageType>("312", RPL_WHOISSERVER));
    _messageMap.insert(pair<string, MessageType>("313", RPL_WHOISOPERATOR));
    _messageMap.insert(pair<string, MessageType>("317", RPL_WHOISIDLE));
    _messageMap.insert(pair<string, MessageType>("319", RPL_WHOISCHANNELS));
    _messageMap.insert(pair<string, MessageType>("378", RPL_WHOISHOST));
    _messageMap.insert(pair<string, MessageType>("401", ERR_NOSUCHNICK));
    _messageMap.insert(pair<string, MessageType>("412", ERR_NOTEXTTOSEND));
    _messageMap.insert(pair<string, MessageType>("318", RPL_ENDOFWHOIS));
    _messageMap.insert(pair<string, MessageType>("482", ERR_CHANOPRIVSNEEDED));
    _messageMap.insert(pair<string, MessageType>("RPL_NOTOPIC", RPL_NOTOPIC));
    _messageMap.insert(pair<string, MessageType>("JOIN", USERJOIN));
    _messageMap.insert(pair<string, MessageType>("QUIT", USERQUIT));
    _messageMap.insert(pair<string, MessageType>("PART", USERPART));
    _messageMap.insert(pair<string, MessageType>("ERROR", ERROR));        
    return true;
}

unsigned IRCInMessage::getParamCount() const
{
    return _tokens.size() - 1;
}
string IRCInMessage::getLastParams(unsigned index, bool clearColon) const
{
    string retVal = "";
    for (unsigned i = index; i < _tokens.size(); ++i) {
        if (!retVal.empty())
            retVal += " " + _tokens[i];
        else if (clearColon) // strip the colon at the beginning
            retVal = _tokens[i].substr(1, _tokens[i].size() - 1);
        else // include the colon at the beginning
            retVal = _tokens[i];
    }
    return retVal;
}
