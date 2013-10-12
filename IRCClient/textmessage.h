/* Author: Balazs Nemeth
 * Description: This class represents a message from a user at a given time
 */

#ifndef TEXTMESSAGE_H
#define TEXTMESSAGE_H

#include <string>
#include <sys/time.h>
#include <unistd.h>

using namespace std;


class TextMessage
{
public:
    // distinguis between three types of messages
    enum MessageType {STDMSG, ERROR, INFO, DIRECTEDMSG, OWNMESSAGE};
    // creates an instance of TextMessage
    TextMessage(const string& nick, const string& message, MessageType msgType = STDMSG);
    /* thanks to this constructor a string can be passed to
        functions that require a TextMessage (and a TextMessage instance will be created implicitly) */
    TextMessage(const string& message, MessageType msgType = STDMSG);
    string getNick() const {return _nick;}
    string getMessage() const {return _message;}
    string getTime() const;
    // returns true if the message starts with [0]
    bool isCmd() const {return _message[0] == '/';}
    // returns the nick from the string passed
    static string parseNick(const string& nick);
    bool contains(const string& str, bool caseSensitive = true) const;
    // returns the whole message as seen by the user
    string toString() const;
    MessageType getMessageType() const {return _msgType;}
    // a type check that returns the same as the other function unless it's type is std and the string is matched in the message
    MessageType getMessageType(const string& nick) const;
    void setMessageType(MessageType msgType) {_msgType = msgType;}
    static void setGMTOffset(int offset = 0);
private:
    MessageType _msgType;
    string _nick;
    string _message;
    time_t _seconds;
    static int _gmtOffset;
};

#endif // TEXTMESSAGE_H
