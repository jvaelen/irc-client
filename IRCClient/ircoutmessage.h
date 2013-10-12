/* Author: Balazs Nemeth
 * Description: IRCOutMessage if used to parse messages sent from the GUI easely into IRC protocl messages
 */


#ifndef IRCOUTMESSAGE_H
#define IRCOUTMESSAGE_H

#include <map>
#include <string>
#include <vector>
#include "textmessage.h"

using namespace std;

class IRCOutMessage
{
public:
    enum MessageType {UNSUPPORTED       = 0,
                      JOIN,
                      PRIVMSG, // either to user or to channel
                      PART,
                      WHOIS,
                      CONNECT,
                      DISCONNECT,
                      MODE,
                      INVITE,
                      NICK,
                      KICK,
                      QUIT,
                      TOPIC,
                      HELP};
    IRCOutMessage(const string& message);
    static bool initMap();
    MessageType getType() const { return _msgType; }
    string getParam() const { return _param; }
    string getLastParams() const { return _lastParams; }
    bool hasExtraParams() const { return _extraParams; }
    // returns the list of available commands
    static void getHelp(vector<TextMessage>& container);
private:
    static bool _initDone;
    MessageType _msgType;
    bool _extraParams;
    static map<string, MessageType> _messageMap;
    string _param;
    string _lastParams;
};

#endif // IRCOUTMESSAGE_H
