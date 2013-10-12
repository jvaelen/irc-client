#include "textmessage.h"
#include "time.h"
#include <sstream>
#include <algorithm>
#include <QDebug>

int TextMessage::_gmtOffset = 0;

TextMessage::TextMessage(const string &nick, const string &message, MessageType msgType) : _msgType(msgType), _nick(nick), _message(message)
{
    // the creation of the object servers as a time specifier
    struct timeval tv;
    gettimeofday(&tv, NULL);
    _seconds = tv.tv_sec;
    _nick = parseNick(nick);
}


string TextMessage::parseNick(const string& nick)
{
    size_t pos;

    // parse out the nick from the nick parameter
    if ((pos = nick.find('!')) != (unsigned) -1)
        return nick.substr(0, pos);
    else
        return nick;
}

bool TextMessage::contains(const string &str, bool caseSensitive) const
{
    if (!caseSensitive) {
        string tempStr = _message;
        string toFind = str;
        std::transform(tempStr.begin(), tempStr.end(),tempStr.begin(), ::toupper);
        std::transform(toFind.begin(), toFind.end(),toFind.begin(), ::toupper);
        return (tempStr.find(toFind) != string::npos);
    }
    else
        return (_message.find(str) != string::npos);
}
TextMessage::MessageType TextMessage::getMessageType(const string& nick) const
{
    if (getMessageType() == TextMessage::STDMSG && contains(nick , false))
        return DIRECTEDMSG;
    else
        return getMessageType();
}


TextMessage::TextMessage(const string &message, MessageType msgType) : _msgType(msgType), _message(message)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    _seconds = tv.tv_sec;
}

string TextMessage::getTime() const
{
    struct tm* temptm = gmtime( &_seconds);
    stringstream ss;
    ss << (temptm->tm_hour + _gmtOffset) % 24 << ":";
    if (temptm->tm_min < 10)
        ss << 0 << temptm->tm_min;
    else
        ss << temptm->tm_min;
    return ss.str();
}

void TextMessage::setGMTOffset(int offset)
{
    _gmtOffset = offset % 24;
}

string TextMessage::toString() const
{
    if (_nick.empty())
        return "[" + getTime() + "] " + _message;
    else
        return "[" + getTime() + "] <" + _nick + "> " + _message;
}
